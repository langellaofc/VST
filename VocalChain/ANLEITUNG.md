# Vocal Chain – Anleitung (Mac)

## Variante A: Automatisch bauen lassen (kein Programmieren nötig)
1. Kostenloses Konto auf github.com anlegen.
2. Neues **privates** Repository erstellen (Button "New").
3. Den kompletten Inhalt dieses Ordners hochladen (inkl. dem versteckten Ordner `.github`).
   Tipp: Am einfachsten mit der kostenlosen App "GitHub Desktop".
4. Im Repository oben auf **Actions** klicken. Der Build startet automatisch (ca. 10–15 Min.).
5. Fertigen Lauf anklicken, unten bei **Artifacts** `VocalChain-mac` herunterladen und entpacken.
6. Kopieren:
   - `Vocal Chain.vst3`  nach  `~/Library/Audio/Plug-Ins/VST3/`
   - `Vocal Chain.component` nach `~/Library/Audio/Plug-Ins/Components/` (für Logic Pro)
7. Im Terminal einmal ausführen (entfernt die Download-Sperre von macOS):
   ```
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Vocal Chain.vst3"
   xattr -cr ~/Library/Audio/Plug-Ins/Components/"Vocal Chain.component"
   ```
8. DAW neu starten bzw. Plugins neu scannen. Das Plugin liegt als Effekt unter "Vocal Chain".

## Variante B: Selbst bauen (Xcode Command Line Tools + CMake nötig)
```
xcode-select --install
brew install cmake
cd VocalChain
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 4
```
Danach wird das Plugin automatisch in deine Plugin-Ordner kopiert.

## Aufbau der Kette (von oben nach unten im Signalfluss)
Gate → Kompressor → EQ (Low Cut + Air) → De-Esser → Pitch → Chorus → Echo → Reverb → Output
Jedes Modul hat oben einen An/Aus-Schalter. Standardmäßig an: Kompressor, EQ, De-Esser.
