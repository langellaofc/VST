#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;
using Attr  = juce::AudioParameterFloatAttributes;

static APVTS::ParameterLayout createLayout()
{
    APVTS::ParameterLayout l;
    auto B = [&] (const char* id, const char* name, bool def)
    { l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def)); };
    auto F = [&] (const char* id, const char* name, float lo, float hi, float def, const char* unit, float skew = 1.0f)
    { l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name,
             juce::NormalisableRange<float> (lo, hi, 0.01f, skew), def, Attr().withLabel (unit))); };

    B ("gate_on", "Gate On", false);
    F ("gate_thresh", "Gate Threshold", -80.0f, 0.0f, -50.0f, " dB");

    B ("comp_on", "Comp On", true);
    F ("comp_thresh", "Comp Threshold", -40.0f, 0.0f, -18.0f, " dB");
    F ("comp_ratio", "Comp Ratio", 1.0f, 10.0f, 3.0f, ":1");
    F ("comp_makeup", "Comp Makeup", 0.0f, 12.0f, 3.0f, " dB");

    B ("eq_on", "EQ On", true);
    F ("eq_lowcut", "Low Cut", 20.0f, 300.0f, 80.0f, " Hz", 0.5f);
    F ("eq_air", "Air", 0.0f, 9.0f, 3.0f, " dB");

    B ("deess_on", "DeEss On", true);
    F ("deess_thresh", "DeEss Threshold", -40.0f, 0.0f, -25.0f, " dB");

    B ("pitch_on", "Pitch On", false);
    F ("pitch_semi", "Pitch", -12.0f, 12.0f, 12.0f, " st");
    F ("pitch_mix", "Pitch Mix", 0.0f, 1.0f, 0.25f, "");

    B ("chorus_on", "Chorus On", false);
    F ("chorus_rate", "Chorus Rate", 0.1f, 5.0f, 0.8f, " Hz");
    F ("chorus_depth", "Chorus Depth", 0.0f, 1.0f, 0.3f, "");
    F ("chorus_mix", "Chorus Mix", 0.0f, 1.0f, 0.3f, "");

    B ("delay_on", "Echo On", false);
    F ("delay_time", "Echo Time", 50.0f, 1000.0f, 350.0f, " ms");
    F ("delay_fb", "Echo Feedback", 0.0f, 0.9f, 0.35f, "");
    F ("delay_mix", "Echo Mix", 0.0f, 1.0f, 0.2f, "");

    B ("reverb_on", "Reverb On", false);
    F ("reverb_size", "Reverb Size", 0.0f, 1.0f, 0.5f, "");
    F ("reverb_damp", "Reverb Damping", 0.0f, 1.0f, 0.5f, "");
    F ("reverb_mix", "Reverb Mix", 0.0f, 1.0f, 0.2f, "");

    F ("out_gain", "Output", -24.0f, 12.0f, 0.0f, " dB");
    return l;
}

VocalChainProcessor::VocalChainProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool VocalChainProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return out == l.getMainInputChannelSet();
}

void VocalChainProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                  (juce::uint32) getTotalNumOutputChannels() };

    gate.prepare (spec);  gate.setAttack (1.0f); gate.setRelease (120.0f); gate.setRatio (10.0f);
    comp.prepare (spec);  comp.setAttack (5.0f); comp.setRelease (80.0f);
    lowCut.prepare (spec); air.prepare (spec); deessFilter.prepare (spec);
    *lowCut.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sampleRate, 80.0f);
    *air.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (
        sampleRate, 10000.0f, 0.7f, juce::Decibels::decibelsToGain (3.0f));
    *deessFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sampleRate, 6000.0f);
    chorus.prepare (spec); chorus.setCentreDelay (7.0f); chorus.setFeedback (0.0f);

    delayLine.setMaximumDelayInSamples ((int) (sampleRate * 1.5) + 4);
    delayLine.prepare (spec);
    delayLine.reset();

    for (auto& s : shifters) s.prepare (sampleRate);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    scratch.setSize (2, juce::jmax (samplesPerBlock, 512));
    deEnv = 0.0f;
    delayLp[0] = delayLp[1] = 0.0f;
}

void VocalChainProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = juce::jmin (buffer.getNumChannels(), 2);
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // 1) Gate
    if (on ("gate_on")) { gate.setThreshold (get ("gate_thresh")); gate.process (ctx); }

    // 2) Kompressor
    if (on ("comp_on"))
    {
        comp.setThreshold (get ("comp_thresh"));
        comp.setRatio (get ("comp_ratio"));
        comp.process (ctx);
        buffer.applyGain (juce::Decibels::decibelsToGain (get ("comp_makeup")));
    }

    // 3) EQ: Low Cut + Air (High Shelf)
    if (on ("eq_on"))
    {
        *lowCut.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sr, get ("eq_lowcut"));
        *air.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf (
            sr, 10000.0f, 0.7f, juce::Decibels::decibelsToGain (get ("eq_air")));
        lowCut.process (ctx);
        air.process (ctx);
    }

    // 4) De-Esser
    if (on ("deess_on"))
    {
        if (scratch.getNumSamples() < n || scratch.getNumChannels() < nch)
            scratch.setSize (juce::jmax (2, nch), n, false, false, true);
        for (int c = 0; c < nch; ++c) scratch.copyFrom (c, 0, buffer, c, 0, n);

        *deessFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass (sr, 6000.0f);
        juce::dsp::AudioBlock<float> sb (scratch);
        auto sub = sb.getSubBlock (0, (size_t) n).getSubsetChannelBlock (0, (size_t) nch);
        juce::dsp::ProcessContextReplacing<float> sctx (sub);
        deessFilter.process (sctx);

        const float thr = juce::Decibels::decibelsToGain (get ("deess_thresh"));
        const float att = 1.0f - std::exp (-1.0f / (0.001f * (float) sr));
        const float rel = 1.0f - std::exp (-1.0f / (0.05f * (float) sr));
        for (int i = 0; i < n; ++i)
        {
            float level = 0.0f;
            for (int c = 0; c < nch; ++c) level = juce::jmax (level, std::abs (scratch.getSample (c, i)));
            deEnv += (level > deEnv ? att : rel) * (level - deEnv);
            const float gr = deEnv > thr ? thr / deEnv : 1.0f;
            for (int c = 0; c < nch; ++c)
            {
                float hi = scratch.getSample (c, i);
                buffer.setSample (c, i, buffer.getSample (c, i) - hi * (1.0f - gr));
            }
        }
    }

    // 5) Pitch / Harmony-Layer
    if (on ("pitch_on"))
    {
        const float ratio = std::pow (2.0f, get ("pitch_semi") / 12.0f);
        const float mix = get ("pitch_mix");
        for (int c = 0; c < nch; ++c)
        {
            auto* d = buffer.getWritePointer (c);
            for (int i = 0; i < n; ++i)
                d[i] += mix * shifters[c].process (d[i], ratio);
        }
    }

    // 6) Chorus
    if (on ("chorus_on"))
    {
        chorus.setRate (get ("chorus_rate"));
        chorus.setDepth (get ("chorus_depth"));
        chorus.setMix (get ("chorus_mix"));
        chorus.process (ctx);
    }

    // 7) Echo
    if (on ("delay_on"))
    {
        const float dSamples = juce::jlimit (1.0f, (float) sr * 1.4f, get ("delay_time") * 0.001f * (float) sr);
        const float fb = get ("delay_fb"), mix = get ("delay_mix");
        for (int c = 0; c < nch; ++c)
        {
            auto* d = buffer.getWritePointer (c);
            for (int i = 0; i < n; ++i)
            {
                const float delayed = delayLine.popSample (c, dSamples);
                delayLp[c] += 0.45f * (delayed - delayLp[c]);   // Echos werden dunkler
                delayLine.pushSample (c, d[i] + delayLp[c] * fb);
                d[i] += delayed * mix;
            }
        }
    }

    // 8) Reverb
    if (on ("reverb_on"))
    {
        juce::Reverb::Parameters p;
        p.roomSize = get ("reverb_size");
        p.damping = get ("reverb_damp");
        p.wetLevel = get ("reverb_mix");
        p.dryLevel = 1.0f - 0.5f * get ("reverb_mix");
        p.width = 1.0f;
        reverb.setParameters (p);
        if (nch == 2) reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
        else          reverb.processMono (buffer.getWritePointer (0), n);
    }

    // Output
    buffer.applyGain (juce::Decibels::decibelsToGain (get ("out_gain")));
}

juce::AudioProcessorEditor* VocalChainProcessor::createEditor() { return new VocalChainEditor (*this); }

void VocalChainProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void VocalChainProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VocalChainProcessor(); }
