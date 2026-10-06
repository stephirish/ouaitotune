#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// MY OUAI look: black, peach (#EEC1A5), smiley grid; face on the left opens its mouth while singing.
class OuaiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OuaiLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    juce::Font getLabelFont (juce::Label&) override;
    static const juce::Colour peach, black, dark;
};

class OuaitotuneEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OuaitotuneEditor (OuaitotuneProcessor&);
    ~OuaitotuneEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    struct Knob
    {
        juce::Slider slider;
        juce::Label name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    void setupKnob (Knob&, const juce::String& id, const juce::String& title);

    OuaitotuneProcessor& proc;
    OuaiLookAndFeel lnf;
    juce::Image faces[6], panel;
    int frame = 0;
    Knob seuil, retune, octave, ouai, dry;
    juce::TextButton snap { "Snap" }, relance { "Relance" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> snapAtt, relAtt;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OuaitotuneEditor)
};
