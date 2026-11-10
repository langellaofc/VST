#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include "Presets.h"

class MixChannelProcessor : public juce::AudioProcessor
{
public:
    MixChannelProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mix Channel"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

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

    using Filt = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                juce::dsp::IIR::Coefficients<float>>;
    double sr = 44100.0;
    Filt hp1, hp2, lp, shelfHi, shelfLo, mud;
    std::array<Filt, 3> band;
    juce::dsp::Compressor<float> comp;

    juce::AudioBuffer<float> dryBuf;
    float peakEnv = 0.0f;   // langsamer Spitzenpegel der Eingangsspur
    float msIn = 0.0f, msOut = 0.0f, lmGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixChannelProcessor)
};
