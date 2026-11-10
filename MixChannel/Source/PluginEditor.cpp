#include "PluginEditor.h"

MixChannelEditor::MixChannelEditor (MixChannelProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    // Rollen-Auswahl
    int id = 1;
    for (auto& r : rolePresets()) roleBox.addItem (r.name, id++);
    addAndMakeVisible (roleBox);
    roleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "role", roleBox);
    roleBox.onChange = [this] { updateTip(); };

    tip.setJustificationType (juce::Justification::topLeft);
    tip.setFont (juce::FontOptions (14.0f));
    tip.setColour (juce::Label::textColourId, juce::Colour (0xffb9c0cc));
    addAndMakeVisible (tip);

    addAndMakeVisible (levelMatch);
    lmAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "levelmatch", levelMatch);

    struct K { juce::String label; const char* id; };
    const std::vector<K> ks {
        { "Amount", "amount" },
        { juce::String::fromUTF8 ("Klarheit"), "clarity" },
        { juce::String::fromUTF8 ("Wärme"), "warmth" },
        { "Punch", "punch" },
        { "Output", "out_gain" },
    };
    for (auto& k : ks)
    {
        auto kn = std::make_unique<Knob>();
        kn->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        kn->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        kn->label.setText (k.label, juce::dontSendNotification);
        kn->label.setJustificationType (juce::Justification::centred);
        kn->label.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        kn->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, k.id, kn->slider);
        addAndMakeVisible (kn->slider);
        addAndMakeVisible (kn->label);
        knobs.push_back (std::move (kn));
    }

    updateTip();
    setSize (640, 400);
}

void MixChannelEditor::updateTip()
{
    const auto& p = rolePresets();
    const int i = juce::jlimit (0, (int) p.size() - 1, roleBox.getSelectedItemIndex());
    tip.setText (juce::String::fromUTF8 (p[(size_t) i].tip), juce::dontSendNotification);
}

void MixChannelEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1d22));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("MIX CHANNEL", 16, 8, 300, 30, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff272a31));
    g.fillRoundedRectangle (juce::Rectangle<float> (12.0f, 150.0f, (float) getWidth() - 24.0f, 190.0f), 8.0f);
}

void MixChannelEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    area.removeFromTop (34);
    roleBox.setBounds (area.removeFromTop (34));
    area.removeFromTop (8);
    tip.setBounds (area.removeFromTop (60));
    area.removeFromTop (10);

    auto row = area.removeFromTop (180);
    const int kw = row.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto kb = row.removeFromLeft (kw);
        k->label.setBounds (kb.removeFromTop (22));
        k->slider.setBounds (kb);
    }
    area.removeFromTop (12);
    levelMatch.setBounds (area.removeFromTop (28));
}
