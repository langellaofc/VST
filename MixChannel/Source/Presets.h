#pragma once
#include <vector>

// Ein EQ-Band: Frequenz (Hz), Gain (dB), Güte (Q)
struct PBand { float f, g, q; };

// Ein Preset für eine Rolle (EDM)
//  hp/lp      : Hochpass / Tiefpass in Hz (lp >= 19000 = aus)
//  b[3]       : drei EQ-Bänder
//  depth      : Kompressor-Schwelle liegt so viele dB unter dem Spitzenpegel der Spur
//  ratio/att/rel : Kompressor (Ratio, Attack ms, Release ms)
//  drive      : Sättigung (1 = sehr leicht)
//  width      : Stereobreite (0 = Mono, 1 = unverändert, >1 = breiter)
struct RolePreset
{
    const char* name;
    const char* tip;
    float hp, lp;
    PBand b[3];
    float depth, ratio, att, rel, drive, width;
};

inline const std::vector<RolePreset>& rolePresets()
{
    static const std::vector<RolePreset> p
    {
        { "Kick",
          "Kick: Mono und tief. Tipp: Lege einen Sidechain-Kompressor auf den Bass, damit Kick und Bass nicht kollidieren.",
          30.0f, 20000.0f, { { 60.0f, 2.5f, 1.0f }, { 300.0f, -3.0f, 1.2f }, { 3500.0f, 2.0f, 1.0f } },
          8.0f, 3.0f, 15.0f, 80.0f, 1.5f, 0.0f },

        { "Sub Bass",
          "Sub Bass: Nur tiefe Frequenzen, immer Mono. Pegel zuerst auf etwa -10 dB bringen, dann mit der Kick abstimmen.",
          25.0f, 200.0f, { { 50.0f, 1.5f, 0.8f }, { 120.0f, -1.0f, 1.0f }, { 250.0f, -2.0f, 1.0f } },
          8.0f, 3.0f, 20.0f, 120.0f, 1.8f, 0.0f },

        { "Mid Bass / Reese",
          "Mid Bass: Der Charakter-Bass. Er braucht Platz in den Mitten, schneide bei Bedarf bei den Leads nach.",
          60.0f, 12000.0f, { { 200.0f, -2.0f, 1.0f }, { 800.0f, 1.5f, 1.0f }, { 3000.0f, 1.5f, 0.8f } },
          10.0f, 4.0f, 10.0f, 100.0f, 2.0f, 0.8f },

        { "Snare / Clap",
          "Snare/Clap: Sitzt zwischen Kick und Hats. Ein kurzer Hall (Reverb) macht sie breiter und lebendiger.",
          120.0f, 20000.0f, { { 200.0f, 2.0f, 1.0f }, { 500.0f, -2.5f, 1.5f }, { 5000.0f, 2.5f, 0.8f } },
          8.0f, 3.0f, 5.0f, 60.0f, 1.3f, 1.1f },

        { "Hi-Hats",
          "Hi-Hats: Luft und Rhythmus. Sie sollten leise sein, aber hörbar. Zu laut klingt der Mix schnell nervig.",
          400.0f, 20000.0f, { { 3000.0f, -1.5f, 1.0f }, { 8000.0f, 1.5f, 0.7f }, { 12000.0f, 1.5f, 0.7f } },
          9.0f, 2.0f, 3.0f, 50.0f, 1.0f, 1.2f },

        { "Percussion / Loops",
          "Percussion: Füllt den Groove auf. Pan sie ruhig nach links oder rechts, damit die Mitte frei bleibt.",
          200.0f, 20000.0f, { { 400.0f, -2.0f, 1.0f }, { 2000.0f, 1.5f, 1.0f }, { 7000.0f, 1.5f, 0.7f } },
          9.0f, 2.5f, 8.0f, 80.0f, 1.2f, 1.2f },

        { "Lead Synth",
          "Lead: Das Hauptmotiv, soll vorne stehen. Wenn es mit dem Vocal kämpft, den Lead bei 2-4 kHz leicht absenken.",
          150.0f, 20000.0f, { { 300.0f, -2.5f, 1.0f }, { 2500.0f, 2.0f, 0.9f }, { 9000.0f, 1.5f, 0.7f } },
          9.0f, 3.0f, 15.0f, 100.0f, 1.4f, 1.1f },

        { "Pad / Atmosphere",
          "Pad: Hintergrund und Breite. Es soll den Mix tragen, aber nicht auffallen. Ruhig leiser als gedacht.",
          200.0f, 14000.0f, { { 300.0f, -3.0f, 0.8f }, { 1500.0f, -1.5f, 0.8f }, { 8000.0f, 1.0f, 0.6f } },
          10.0f, 2.0f, 30.0f, 200.0f, 1.0f, 1.4f },

        { "Pluck / Arp",
          "Pluck/Arp: Rhythmische Melodie. Ein Echo (Delay) und etwas Panning machen sie groß.",
          200.0f, 20000.0f, { { 350.0f, -2.5f, 1.0f }, { 3000.0f, 2.0f, 1.0f }, { 10000.0f, 1.5f, 0.7f } },
          9.0f, 2.5f, 8.0f, 90.0f, 1.3f, 1.2f },

        { "Vocal",
          "Vocal: Muss im Mix klar verständlich sein. Dafür den Lead und die Pads bei 2-4 kHz leicht absenken.",
          100.0f, 20000.0f, { { 250.0f, -2.0f, 1.0f }, { 3000.0f, 2.0f, 0.9f }, { 10000.0f, 2.0f, 0.6f } },
          10.0f, 3.5f, 8.0f, 90.0f, 1.2f, 1.0f },

        { "FX / Riser",
          "FX/Riser: Für Übergänge. Breit und leise halten, damit sie den Drop nicht überdecken.",
          250.0f, 18000.0f, { { 500.0f, -2.0f, 1.0f }, { 4000.0f, 1.0f, 0.8f }, { 10000.0f, 1.0f, 0.7f } },
          10.0f, 2.0f, 20.0f, 150.0f, 1.0f, 1.5f },

        { "Drum Bus",
          "Drum Bus: Auf die Gruppe aller Drums legen, damit sie als Einheit klingen (Glue).",
          30.0f, 20000.0f, { { 100.0f, 1.5f, 0.8f }, { 400.0f, -1.5f, 1.0f }, { 8000.0f, 1.5f, 0.6f } },
          6.0f, 2.0f, 25.0f, 100.0f, 1.3f, 1.0f },

        { "Music Bus",
          "Music Bus: Auf die Gruppe aller Synths und Melodien legen, damit sie zusammenpassen.",
          30.0f, 20000.0f, { { 300.0f, -1.0f, 1.0f }, { 3000.0f, 1.0f, 0.8f }, { 12000.0f, 1.0f, 0.6f } },
          6.0f, 1.8f, 30.0f, 150.0f, 1.1f, 1.1f },
    };
    return p;
}
