#pragma once
#include "PluginProcessor.h"

class VocalChainEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalChainEditor (VocalChainProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Section
    {
        juce::ToggleButton toggle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
        std::vector<std::unique_ptr<Knob>> knobs;
    };

    VocalChainProcessor& proc;
    std::vector<std::unique_ptr<Section>> sections;
    std::vector<juce::Rectangle<int>> cells;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalChainEditor)
};
