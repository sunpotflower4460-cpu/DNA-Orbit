# Licensing Notes

DNA Orbit is built on top of the JUCE framework (https://juce.com), pinned to
tag `8.0.15` and fetched via CMake `FetchContent` (see `CMakeLists.txt`).

JUCE is dual-licensed:

- **JUCE AGPLv3 / Personal / Educational** tiers: free to use, but distributing
  a built plugin under these tiers carries specific obligations (e.g. AGPLv3
  requires releasing full corresponding source, including this project, under
  a compatible license).
- **JUCE Indie / Pro** commercial tiers: paid, remove the AGPLv3 / attribution
  obligations and the JUCE splash screen requirement, and are required once
  revenue/funding thresholds set by JUCE are exceeded.

**Before distributing a built copy of DNA Orbit (VST3/AU/Standalone) outside
of personal/internal testing, confirm which JUCE license tier applies to your
situation** at https://juce.com/get-juce and ensure the corresponding
obligations are met (attribution, splash screen, source availability, or a
paid license, as applicable). This project does not include a JUCE
commercial license and leaves JUCE's splash-screen requirement at its
default (enabled) so it stays compliant with the free tiers; only disable it
if you hold a paid JUCE license that permits doing so.

DNA Orbit's own source code (everything under `Source/` and `Tests/`) is
provided as-is; add your preferred license text for that code separately
before distributing.
