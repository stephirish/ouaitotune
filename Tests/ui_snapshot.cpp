#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_gui_basics/juce_gui_basics.h>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main (int, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
    auto* proc = dynamic_cast<OuaitotuneProcessor*> (p.get());
    for (int lv : { 0, 5 })
    {
        proc->dsp.level.store (lv / 5.0f);
        std::unique_ptr<OuaitotuneEditor> ed (static_cast<OuaitotuneEditor*> (p->createEditor()));
        ed->setVisible (true);
        ed->setBounds (0, 0, 460, 169);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 2.0f);
        juce::File f (juce::String (argv[1]) + "_" + juce::String (lv) + ".png"); f.deleteFile();
        juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    puts ("snap ok");
    return 0;
}
