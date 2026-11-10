#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

// Einfacher Pitch-Shifter (zwei überblendete Delay-Taps)
struct PitchShifter
{
    std::vector<float> buf;
    int size = 0, w = 0;
    float phase = 0.0f, win = 2400.0f;

    void prepare (double sr)
    {
        win = (float) (0.05 * sr);
        size = juce::nextPowerOfTwo ((int) (win * 2.0f) + 8);
        buf.assign ((size_t) size, 0.0f);
        w = 0; phase = 0.0f;
    }

    float read (float delay) const
    {
        float rp = (float) w - delay;
        while (rp < 0.0f) rp += (float) size;
        int i0 = (int) rp;
        float fr = rp - (float) i0;
        int i1 = (i0 + 1) % size;
        i0 %= size;
        return buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;
    }

    float process (float in, float ratio)
    {
        buf[(size_t) w] = in;
        phase += (1.0f - ratio) / win;
        phase -= std::floor (phase);
        float p2 = phase + 0.5f; p2 -= std::floor (p2);
        float g1 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase);
        float g2 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * p2);
        float out = read (phase * win + 1.0f) * g1 + read (p2 * win + 1.0f) * g2;
        w = (w + 1) % size;
        return out;
    }
};

class VocalChainProcessor : public juce::AudioProcessor
{
public:
    VocalChainProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Vocal Chain"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    float get (const char* id) const { return apvts.getRawParameterValue (id)->load(); }
    bool on (const char* id) const { return get (id) > 0.5f; }

    double sr = 44100.0;

    juce::dsp::NoiseGate<float> gate;
    juce::dsp::Compressor<float> comp;

    using Filt = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    Filt lowCut, air, deessFilter;

    juce::AudioBuffer<float> scratch;
    float deEnv = 0.0f;

    PitchShifter shifters[2];
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    float delayLp[2] { 0.0f, 0.0f };
    juce::Reverb reverb;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalChainProcessor)
};
