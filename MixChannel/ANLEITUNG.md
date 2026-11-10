# Mix Channel – Anleitung

Installation wie beim Vocal Chain:
1. Ordner in ein (privates) GitHub-Repository hochladen (inkl. `.github`).
2. Unter "Actions" den Build abwarten, Artifact `MixChannel-mac` laden und entpacken.
3. `Mix Channel.vst3` nach `~/Library/Audio/Plug-Ins/VST3/`, `Mix Channel.component` nach `~/Library/Audio/Plug-Ins/Components/`.
4. Im Terminal:
   xattr -cr ~/Library/Audio/Plug-Ins/VST3/"Mix Channel.vst3"
   xattr -cr ~/Library/Audio/Plug-Ins/Components/"Mix Channel.component"
5. DAW neu starten / Plugins neu scannen.

## So benutzt du es (Anfänger-Workflow)
1. Mix Channel auf jede Spur legen und oben die passende **Rolle** wählen (z. B. Kick, Sub Bass, Lead Synth).
2. **Amount** auf 100 % lassen und die Spur im Gesamtmix hören. Der Tipp-Text unter der Rolle hilft dir.
3. Mit **Klarheit**, **Wärme** und **Punch** nach Gehör feinjustieren. Mit Amount das Ganze abschwächen.
4. **Level Match** angeschaltet lassen: So hörst du nur den Klangunterschied, nicht "lauter = besser".
5. Danach Lautstärken mit den Fadern der DAW einstellen (Reihenfolge: Kick, Bass, Snare, dann Rest).
