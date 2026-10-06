# Ouaitotune (VST3 / AU)

Le Ouaitotune en plugin : il suit la hauteur de la voix qui entre et la rechante avec le « Ouaiiii » de Ouai Stéphane.
Même algorithme que le device Max for Live, réécrit en C++ (JUCE). Le sample est intégré au plugin.

## Réglages
- **Seuil** : volume à partir duquel la voix est prise en compte
- **Retune** : vitesse de correction (0 = effet autotune robotique)
- **Octave** : -2 à +2
- **Ouai** / **Dry** : volumes du Ouai et de la voix d'origine
- **Snap** : cale sur les demi-tons justes
- **Relance** : un nouveau « Ouai » à chaque note

## Installer sur Mac
1. Copier `Ouaitotune.vst3` dans `~/Library/Audio/Plug-Ins/VST3/`
   et `Ouaitotune.component` dans `~/Library/Audio/Plug-Ins/Components/`
2. Le plugin n'est pas signé par un compte développeur Apple. Si macOS le bloque, ouvrir le Terminal et taper :
   `xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Ouaitotune.vst3 ~/Library/Audio/Plug-Ins/Components/Ouaitotune.component`
3. Dans Ableton : Préférences > Plug-ins > activer « Use VST3 Plug-in System Folders » / « Use Audio Units », puis Rescan.

## Compiler soi-même
- Automatique : pousser ce dossier sur GitHub, l'onglet **Actions** construit les versions Mac (VST3 + AU, Apple Silicon + Intel) et Windows (VST3) à télécharger.
- À la main (Mac, Xcode + CMake installés) :
  `cmake -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" && cmake --build build --config Release`

## Fichiers
- `Source/OuaitotuneDSP.h` : le moteur (suivi de hauteur YIN + lecture du Ouai en boucle invisible)
- `Source/PluginProcessor.*`, `Source/PluginEditor.*` : le plugin et son interface MY OUAI
- `Tests/offline_test.cpp` : test sans DAW (chante une mélodie dans le moteur et vérifie les notes)
