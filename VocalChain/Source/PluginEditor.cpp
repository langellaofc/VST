#include "PluginEditor.h"

VocalChainEditor::VocalChainEditor (VocalChainProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    struct Spec { const char* title; const char* toggleId; std::vector<std::pair<const char*, const char*>> knobs; };
    const std::vector<Spec> specs
    {
        { "Gate",       "gate_on",   { { "Threshold", "gate_thresh" } } },
        { "Compressor", "comp_on",   { { "Threshold", "comp_thresh" }, { "Ratio", "comp_ratio" }, { "Makeup", "comp_makeup" } } },
        { "EQ",         "eq_on",     { { "Low Cut", "eq_lowcut" }, { "Air", "eq_air" } } },
        { "De-Esser",   "deess_on",  { { "Threshold", "deess_thresh" } } },
        { "Pitch",      "pitch_on",  { { "Semitones", "pitch_semi" }, { "Mix", "pitch_mix" } } },
        { "Chorus",     "chorus_on", { { "Rate", "chorus_rate" }, { "Depth", "chorus_depth" }, { "Mix", "chorus_mix" } } },
        { "Echo",       "delay_on",  { { "Time", "delay_time" }, { "Feedback", "delay_fb" }, { "Mix", "delay_mix" } } },
        { "Reverb",     "reverb_on", { { "Size", "reverb_size" }, { "Damping", "reverb_damp" }, { "Mix", "reverb_mix" } } },
        { "Output",     nullptr,     { { "Level", "out_gain" } } },
    };

    for (auto& s : specs)
    {
        auto sec = std::make_unique<Section>();
        sec->toggle.setButtonText (s.title);
        if (s.toggleId != nullptr)
            sec->att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, s.toggleId, sec->toggle);
        else
        {
            sec->toggle.setToggleState (true, juce::dontSendNotification);
            sec->toggle.setClickingTogglesState (false);
        }
        addAndMakeVisible (sec->toggle);

        for (auto& k : s.knobs)
        {
            auto knob = std::make_unique<Knob>();
            knob->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            knob->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
            knob->label.setText (k.first, juce::dontSendNotification);
            knob->label.setJustificationType (juce::Justification::centred);
            knob->label.setFont (juce::FontOptions (12.0f));
            knob->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, k.second, knob->slider);
            addAndMakeVisible (knob->slider);
            addAndMakeVisible (knob->label);
            sec->knobs.push_back (std::move (knob));
        }
        sections.push_back (std::move (sec));
    }
    setSize (780, 540);
}

void VocalChainEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1b1d22));
    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (22.0f, juce::Font::bold));
    g.drawText ("VOCAL CHAIN", 16, 6, 300, 30, juce::Justification::centredLeft);
    for (auto& c : cells)
    {
        g.setColour (juce::Colour (0xff272a31));
        g.fillRoundedRectangle (c.toFloat(), 8.0f);
    }
}

void VocalChainEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (34);
    const int cols = 3, rows = 3;
    const int w = area.getWidth() / cols, h = area.getHeight() / rows;
    cells.clear();

    for (size_t i = 0; i < sections.size(); ++i)
    {
        auto cell = juce::Rectangle<int> (area.getX() + ((int) i % cols) * w,
                                          area.getY() + ((int) i / cols) * h, w, h).reduced (5);
        cells.push_back (cell);
        auto inner = cell.reduced (6);
        sections[i]->toggle.setBounds (inner.removeFromTop (24));
        const int kw = inner.getWidth() / 3;
        for (auto& k : sections[i]->knobs)
        {
            auto kb = inner.removeFromLeft (kw);
            k->label.setBounds (kb.removeFromBottom (16));
            k->slider.setBounds (kb);
        }
    }
    repaint();
}
