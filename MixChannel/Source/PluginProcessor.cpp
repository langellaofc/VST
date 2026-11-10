#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;
using Attr  = juce::AudioParameterFloatAttributes;
using ArrayCoeffs = juce::dsp::IIR::ArrayCoefficients<float>;

static APVTS::ParameterLayout createLayout()
{
    APVTS::ParameterLayout l;

    juce::StringArray names;
    for (auto& r : rolePresets()) names.add (r.name);
    l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "role", 1 }, "Role", names, 0));

    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "amount", 1 }, "Amount",
           juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, Attr().withLabel (" %")));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "clarity", 1 }, "Clarity",
           juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, Attr().withLabel (" %")));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "warmth", 1 }, "Warmth",
           juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, Attr().withLabel (" %")));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "punch", 1 }, "Punch",
           juce::NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, Attr().withLabel (" %")));
    l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "levelmatch", 1 }, "Level Match", true));
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "out_gain", 1 }, "Output",
           juce::NormalisableRange<float> (-24.0f, 12.0f, 0.01f), 0.0f, Attr().withLabel (" dB")));
    return l;
}

MixChannelProcessor::MixChannelProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool MixChannelProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return out == l.getMainInputChannelSet();
}

void MixChannelProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock,
                                  (juce::uint32) getTotalNumOutputChannels() };
    hp1.prepare (spec); hp2.prepare (spec); lp.prepare (spec);
    shelfHi.prepare (spec); shelfLo.prepare (spec); mud.prepare (spec);
    for (auto& b : band) b.prepare (spec);
    *hp1.state = ArrayCoeffs::makeHighPass (sampleRate, 80.0f);
    *hp2.state = ArrayCoeffs::makeHighPass (sampleRate, 80.0f);
    *lp.state = ArrayCoeffs::makeLowPass (sampleRate, 12000.0f);
    *shelfHi.state = ArrayCoeffs::makeHighShelf (sampleRate, 8000.0f, 0.7f, 1.0f);
    *shelfLo.state = ArrayCoeffs::makeLowShelf (sampleRate, 150.0f, 0.7f, 1.0f);
    *mud.state = ArrayCoeffs::makePeakFilter (sampleRate, 300.0f, 1.0f, 1.0f);
    for (auto& b : band)
        *b.state = ArrayCoeffs::makePeakFilter (sampleRate, 1000.0f, 1.0f, 1.0f);
    comp.prepare (spec);
    dryBuf.setSize (2, juce::jmax (samplesPerBlock, 512));
    peakEnv = 0.0f; msIn = msOut = 0.0f; lmGain = 1.0f;
}

void MixChannelProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nch = juce::jmin (buffer.getNumChannels(), 2);
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);

    const auto& presets = rolePresets();
    const int roleIdx = juce::jlimit (0, (int) presets.size() - 1, (int) std::round (get ("role")));
    const auto& rp = presets[(size_t) roleIdx];

    const float amount = get ("amount") * 0.01f;
    const float clarity = get ("clarity") * 0.01f;
    const float warmth = get ("warmth") * 0.01f;
    const float punch = get ("punch") * 0.01f;
    const bool levelMatch = get ("levelmatch") > 0.5f;
    const float nyq = (float) sr * 0.45f;

    // Eingang merken (für Dry/Wet) und Pegel messen (für Auto-Kompressor und Level-Match)
    if (dryBuf.getNumSamples() < n || dryBuf.getNumChannels() < nch)
        dryBuf.setSize (juce::jmax (2, nch), n, false, false, true);
    for (int c = 0; c < nch; ++c) dryBuf.copyFrom (c, 0, buffer, c, 0, n);

    const float peakDecay = std::exp (-1.0f / (3.0f * (float) sr));
    const float kMs = 1.0f - std::exp (-1.0f / (0.5f * (float) sr));
    for (int i = 0; i < n; ++i)
    {
        float pk = 0.0f, e = 0.0f;
        for (int c = 0; c < nch; ++c)
        {
            const float x = dryBuf.getSample (c, i);
            pk = juce::jmax (pk, std::abs (x));
            e += x * x;
        }
        peakEnv = juce::jmax (pk, peakEnv * peakDecay);
        msIn += kMs * (e / (float) nch - msIn);
    }

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> ctx (block);

    // --- EQ-Kette ---
    *hp1.state = ArrayCoeffs::makeHighPass (sr, juce::jmin (rp.hp, nyq));
    *hp2.state = ArrayCoeffs::makeHighPass (sr, juce::jmin (rp.hp, nyq));
    hp1.process (ctx);
    hp2.process (ctx);

    if (rp.lp < 19000.0f)
    {
        *lp.state = ArrayCoeffs::makeLowPass (sr, juce::jmin (rp.lp, nyq));
        lp.process (ctx);
    }

    for (size_t i = 0; i < 3; ++i)
    {
        *band[i].state = ArrayCoeffs::makePeakFilter (sr, juce::jmin (rp.b[i].f, nyq), rp.b[i].q,
                                  juce::Decibels::decibelsToGain (rp.b[i].g));
        band[i].process (ctx);
    }

    // Makro: Klarheit = Höhen rauf, Matsch (300 Hz) runter
    *shelfHi.state = ArrayCoeffs::makeHighShelf (sr, 8000.0f, 0.7f, juce::Decibels::decibelsToGain (4.0f * clarity));
    *mud.state = ArrayCoeffs::makePeakFilter (sr, 300.0f, 1.0f, juce::Decibels::decibelsToGain (-3.0f * clarity));
    // Makro: Wärme = Tiefmitten rauf (+ mehr Sättigung weiter unten)
    *shelfLo.state = ArrayCoeffs::makeLowShelf (sr, 150.0f, 0.7f, juce::Decibels::decibelsToGain (3.0f * warmth));
    shelfHi.process (ctx);
    mud.process (ctx);
    shelfLo.process (ctx);

    // --- Kompressor (Schwelle folgt automatisch dem Pegel der Spur) ---
    const float peakDb = juce::Decibels::gainToDecibels (peakEnv, -80.0f);
    const float depth = juce::jlimit (3.0f, 20.0f, rp.depth - 3.0f * punch);
    comp.setThreshold (juce::jlimit (-60.0f, 0.0f, peakDb - depth));
    comp.setRatio (juce::jlimit (1.0f, 10.0f, rp.ratio + 2.0f * punch));
    comp.setAttack (juce::jlimit (1.0f, 80.0f, rp.att * (1.0f + punch)));
    comp.setRelease (rp.rel);
    comp.process (ctx);

    // --- Sättigung ---
    const float drive = juce::jmax (1.0f, rp.drive + 1.5f * juce::jmax (0.0f, warmth) + 0.3f * juce::jmin (0.0f, warmth));
    for (int c = 0; c < nch; ++c)
    {
        auto* d = buffer.getWritePointer (c);
        for (int i = 0; i < n; ++i) d[i] = std::tanh (d[i] * drive) / drive;
    }

    // --- Stereobreite (Mid/Side) ---
    if (nch == 2)
    {
        auto* l = buffer.getWritePointer (0);
        auto* r = buffer.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            const float m = 0.5f * (l[i] + r[i]);
            const float s = 0.5f * (l[i] - r[i]) * rp.width;
            l[i] = m + s;
            r[i] = m - s;
        }
    }

    // --- Amount: Mischung aus Original und bearbeitetem Signal ---
    for (int c = 0; c < nch; ++c)
    {
        auto* w = buffer.getWritePointer (c);
        const auto* d = dryBuf.getReadPointer (c);
        for (int i = 0; i < n; ++i) w[i] = d[i] + amount * (w[i] - d[i]);
    }

    // --- Auto-Level-Match: Ausgang so laut wie Eingang ---
    const float kGain = 1.0f - std::exp (-1.0f / (0.3f * (float) sr));
    const float outLin = juce::Decibels::decibelsToGain (get ("out_gain"));
    for (int i = 0; i < n; ++i)
    {
        float e = 0.0f;
        for (int c = 0; c < nch; ++c) { const float x = buffer.getSample (c, i); e += x * x; }
        msOut += kMs * (e / (float) nch - msOut);

        float target = 1.0f;
        if (levelMatch && msIn > 1.0e-7f && msOut > 1.0e-9f)
            target = juce::jlimit (0.25f, 4.0f, std::sqrt (msIn / msOut));
        lmGain += kGain * (target - lmGain);

        for (int c = 0; c < nch; ++c)
            buffer.setSample (c, i, buffer.getSample (c, i) * lmGain * outLin);
    }
}

juce::AudioProcessorEditor* MixChannelProcessor::createEditor() { return new MixChannelEditor (*this); }

void MixChannelProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void MixChannelProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MixChannelProcessor(); }
