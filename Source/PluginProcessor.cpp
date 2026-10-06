#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

namespace ids
{
    static const juce::ParameterID seuil { "seuil", 1 }, retune { "retune", 1 }, octave { "octave", 1 },
                                   ouai { "ouai", 1 }, dry { "dry", 1 }, snap { "snap", 1 }, relance { "relance", 1 };
}

juce::AudioProcessorValueTreeState::ParameterLayout OuaitotuneProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto db = AudioParameterFloatAttributes().withLabel ("dB");
    p.push_back (std::make_unique<AudioParameterFloat> (ids::seuil, "Seuil", NormalisableRange<float> (-70.0f, 0.0f, 0.1f), -40.0f, db));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::retune, "Retune", NormalisableRange<float> (0.0f, 500.0f, 0.1f, 0.4f), 25.0f,
                                                        AudioParameterFloatAttributes().withLabel ("ms")));
    p.push_back (std::make_unique<AudioParameterInt> (ids::octave, "Octave", -2, 2, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::ouai, "Ouai", NormalisableRange<float> (-70.0f, 6.0f, 0.1f), 0.0f, db));
    p.push_back (std::make_unique<AudioParameterFloat> (ids::dry, "Dry", NormalisableRange<float> (-70.0f, 0.0f, 0.1f), -70.0f, db));
    p.push_back (std::make_unique<AudioParameterBool> (ids::snap, "Snap", true));
    p.push_back (std::make_unique<AudioParameterBool> (ids::relance, "Relance", true));
    return { p.begin(), p.end() };
}

OuaitotuneProcessor::OuaitotuneProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "OUAITOTUNE", createLayout())
{
    // the long "Ouaiiii" is compiled into the plugin
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    auto stream = std::make_unique<juce::MemoryInputStream> (BinaryData::Ouai_Long_loop_wav,
                                                             (size_t) BinaryData::Ouai_Long_loop_wavSize, false);
    if (std::unique_ptr<juce::AudioFormatReader> reader { fm.createReaderFor (std::move (stream)) })
    {
        juce::AudioBuffer<float> buf ((int) reader->numChannels, (int) reader->lengthInSamples);
        reader->read (&buf, 0, (int) reader->lengthInSamples, 0, true, true);
        std::vector<float> l (buf.getReadPointer (0), buf.getReadPointer (0) + buf.getNumSamples());
        std::vector<float> r (buf.getReadPointer (buf.getNumChannels() > 1 ? 1 : 0),
                              buf.getReadPointer (buf.getNumChannels() > 1 ? 1 : 0) + buf.getNumSamples());
        dsp.setSample (std::move (l), std::move (r), reader->sampleRate, 2000.0, 12996.0);
    }
}

bool OuaitotuneProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return layouts.getMainInputChannelSet() == out;
}

void OuaitotuneProcessor::pushParams()
{
    dsp.seuilDb = apvts.getRawParameterValue ("seuil")->load();
    dsp.retuneMs = apvts.getRawParameterValue ("retune")->load();
    dsp.octave = (int) apvts.getRawParameterValue ("octave")->load();
    dsp.ouaiDb = apvts.getRawParameterValue ("ouai")->load();
    dsp.dryDb = apvts.getRawParameterValue ("dry")->load();
    dsp.snap = apvts.getRawParameterValue ("snap")->load() > 0.5f;
    dsp.relance = apvts.getRawParameterValue ("relance")->load() > 0.5f;
}

void OuaitotuneProcessor::prepareToPlay (double sampleRate, int)
{
    pushParams();
    dsp.prepare (sampleRate);
}

void OuaitotuneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    pushParams();
    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    dsp.process (L, R, buffer.getNumSamples());
}

void OuaitotuneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void OuaitotuneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* OuaitotuneProcessor::createEditor() { return new OuaitotuneEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new OuaitotuneProcessor(); }
