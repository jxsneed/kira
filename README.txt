KIRA — VST3 source project for Ableton Live (Windows 10/11)

WHAT'S INCLUDED
- JUCE/CMake source project.
- VST3 + Standalone build targets.
- Five main controls: BEND, TUNE, SPEED, VIBE, SMILE.
- Key + scale selection for pitch correction.
- Anime-inspired compact interface.

IMPORTANT
This ZIP is the source project, not a precompiled .vst3 binary.
A native Windows VST3 must be compiled on Windows (or a compatible build machine).
The pitch engine is an experimental lightweight prototype. It is not a transparent,
commercial-grade Auto-Tune replacement. Expect artifacts on polyphonic material,
noisy recordings, very low notes, and fast melodic passages. Best tested on a clean,
monophonic vocal recording.

BUILD ON WINDOWS
1. Install Visual Studio 2022 (Desktop development with C++).
2. Install CMake 3.22+ and Git.
3. Extract this folder.
4. Open PowerShell in this folder and run:

   cmake -B build -S . -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release

5. The VST3 should be created under:
   build/KIRA_artefacts/Release/VST3/KIRA.vst3

6. Copy KIRA.vst3 to:
   C:\Program Files\Common Files\VST3

7. In Ableton: Preferences > Plug-Ins > Rescan.

JUCE is fetched from its official Git repository during CMake configuration,
so the first configure step requires an internet connection.

CONTROL GUIDE
BEND: continuous pitch shift, -12 to +12 semitones.
TUNE: amount of scale-based pitch correction.
SPEED: correction response; higher is faster/more assertive.
VIBE: subtle pitch movement / instability.
SMILE: rising high-pass, echo bloom and saturation macro.

KEY/SCALE: select the vocal key and Chromatic, Major or Minor mode.
For a natural result, start with TUNE around 40–60 and SPEED around 35–55.
For a hard, obvious effect, use TUNE 90–100 and SPEED 85–100.

This is an original prototype project. KIRA is a working name.
