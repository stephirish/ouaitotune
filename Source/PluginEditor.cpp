#include "PluginEditor.h"
#include "BinaryData.h"

const juce::Colour OuaiLookAndFeel::peach { 0xffeec1a5 };
const juce::Colour OuaiLookAndFeel::black { 0xff060302 };
const juce::Colour OuaiLookAndFeel::dark  { 0xff292626 };

static juce::Font mono (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), size,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

OuaiLookAndFeel::OuaiLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, peach);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, peach);
}

void OuaiLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float a0, float a1, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);
    juce::Path bg, val;
    bg.addCentredArc (c.x, c.y, r, r, 0.0f, a0, a1, true);
    val.addCentredArc (c.x, c.y, r, r, 0.0f, a0, ang, true);
    g.setColour (dark);  g.strokePath (bg,  juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (peach); g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colours::white);
    g.drawLine (c.x, c.y, c.x + std::sin (ang) * r * 0.8f, c.y - std::cos (ang) * r * 0.8f, 2.0f);
}

void OuaiLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (b.getToggleState() ? peach : black);
    g.fillRect (r);
    g.setColour (peach);
    g.drawRect (r, 1.0f);
}

void OuaiLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setColour (b.getToggleState() ? black : peach);
    g.setFont (mono (10.5f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
}

juce::Font OuaiLookAndFeel::getLabelFont (juce::Label&) { return mono (10.0f); }

// ----------------------------------------------------------------------------

OuaitotuneEditor::OuaitotuneEditor (OuaitotuneProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    const void* faceData[6] = { BinaryData::face_m0_jpg, BinaryData::face_m1_jpg, BinaryData::face_m2_jpg,
                                BinaryData::face_m3_jpg, BinaryData::face_m4_jpg, BinaryData::face_m5_jpg };
    const int faceSize[6] = { BinaryData::face_m0_jpgSize, BinaryData::face_m1_jpgSize, BinaryData::face_m2_jpgSize,
                              BinaryData::face_m3_jpgSize, BinaryData::face_m4_jpgSize, BinaryData::face_m5_jpgSize };
    for (int k = 0; k < 6; ++k)
        faces[k] = juce::ImageCache::getFromMemory (faceData[k], faceSize[k]);
    panel = juce::ImageCache::getFromMemory (BinaryData::panel_jpg, BinaryData::panel_jpgSize);

    setupKnob (seuil, "seuil", "Seuil");
    setupKnob (retune, "retune", "Retune");
    setupKnob (octave, "octave", "Octave");
    setupKnob (ouai, "ouai", "Ouai");
    setupKnob (dry, "dry", "Dry");
    for (auto* b : { &snap, &relance })
    {
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }
    snapAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "snap", snap);
    relAtt  = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "relance", relance);

    setSize (460, 169);
    startTimerHz (30);
}

OuaitotuneEditor::~OuaitotuneEditor() { setLookAndFeel (nullptr); }

void OuaitotuneEditor::setupKnob (Knob& k, const juce::String& id, const juce::String& title)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 13);
    k.slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    addAndMakeVisible (k.slider);
    k.name.setText (title, juce::dontSendNotification);
    k.name.setJustificationType (juce::Justification::centred);
    k.name.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (k.name);
    k.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}

void OuaitotuneEditor::paint (juce::Graphics& g)
{
    g.fillAll (OuaiLookAndFeel::black);
    if (faces[frame].isValid())
        g.drawImage (faces[frame], juce::Rectangle<float> (0, 0, 200, 169), juce::RectanglePlacement::fillDestination);
    if (panel.isValid())
        g.drawImage (panel, juce::Rectangle<float> (200, 0, 260, 169), juce::RectanglePlacement::fillDestination);
}

void OuaitotuneEditor::resized()
{
    auto place = [] (Knob& k, int x, int y)
    {
        k.name.setBounds (x - 8, y, 60, 12);
        k.slider.setBounds (x - 8, y + 11, 60, 52);
    };
    place (seuil, 210, 24);
    place (retune, 272, 24);
    place (octave, 334, 24);
    place (ouai, 210, 100);
    place (dry, 272, 100);
    snap.setBounds (334, 110, 62, 18);
    relance.setBounds (334, 134, 62, 18);
}

void OuaitotuneEditor::timerCallback()
{
    const float lv = proc.dsp.level.load();
    const int f = juce::jlimit (0, 5, (int) std::lround (lv * 5.4f));
    if (f != frame) { frame = f; repaint (0, 0, 200, 169); }
}
