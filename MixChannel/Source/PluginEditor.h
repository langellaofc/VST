#pragma once
#include "PluginProcessor.h"

class MixChannelEditor : public juce::AudioProcessorEditor
{
public:
    explicit MixChannelEditor (MixChannelProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void updateTip();

    MixChannelProcessor& proc;
    juce::ComboBox roleBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> roleAtt;
    juce::Label tip;
    juce::ToggleButton levelMatch { "Level Match (gleiche Lautstärke wie vorher)" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lmAtt;
    std::vector<std::unique_ptr<Knob>> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixChannelEditor)
};
