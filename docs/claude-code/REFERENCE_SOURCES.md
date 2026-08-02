# Primary Reference Sources

Last reviewed: 2026-08-02

This file is an entry-point registry, not a substitute for reading the current version of each source. APIs, validators, standards, licensing, signing, and Claude Code behavior can change. Recheck the current primary source before making a version-sensitive decision.

## Source-use policy

1. Prefer official documentation, specifications, standards bodies, framework source, and original research.
2. Record the exact version/date used in the relevant ADR or experiment.
3. State which claim the source supports and which claims remain inference.
4. When sources disagree, preserve the disagreement and test the behavior that matters to DNA Orbit.
5. Blogs, forum posts, videos, competitor marketing, and AI answers may identify questions but must not be the sole basis for a physical, API, compatibility, or release claim.

## Claude Code project architecture

- Project memory and imports: <https://code.claude.com/docs/en/memory>
- Skills: <https://code.claude.com/docs/en/skills>
- Subagents: <https://code.claude.com/docs/en/sub-agents>
- Hooks: <https://code.claude.com/docs/en/hooks-guide>
- Settings and permissions: <https://code.claude.com/docs/en/settings>

Use these sources when changing `CLAUDE.md`, `.claude/rules`, `.claude/skills`, `.claude/agents`, `.claude/settings.json`, or optional hooks. Verify frontmatter/tool names against the installed Claude Code version rather than assuming old examples remain valid.

## JUCE and host integration

- `juce::AudioProcessor`: <https://docs.juce.com/master/classjuce_1_1AudioProcessor.html>
- `juce::AudioPlayHead`: <https://docs.juce.com/master/classjuce_1_1AudioPlayHead.html>
- `juce::SmoothedValue`: <https://docs.juce.com/master/classjuce_1_1SmoothedValue.html>
- `juce::dsp::DelayLine`: <https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html>
- JUCE source and releases: <https://github.com/juce-framework/JUCE>

Important current project rule: host playhead information is queried from audio processing callbacks, not from `prepareToPlay`. Recheck JUCE documentation whenever transport or host-position behavior changes.

## Plug-in formats and validation

- Steinberg VST 3 Developer Portal: <https://steinbergmedia.github.io/vst3_dev_portal/>
- Steinberg VST 3 SDK and Validator: <https://github.com/steinbergmedia/vst3sdk>
- pluginval: <https://github.com/Tracktion/pluginval>
- Apple Audio Unit validation (`auval`): use the currently installed Apple developer tools documentation and `auval -h` output.

Validator success is necessary but not sufficient. Real DAW behavior, state restore, transport, automation, bypass, offline rendering, editor lifecycle, and clean-machine scanning still require separate evidence.

## Loudness, level, and true peak

- ITU-R BS.1770-5: <https://www.itu.int/rec/R-REC-BS.1770-5-202311-I>
- EBU Loudness / R128 resources: <https://tech.ebu.ch/loudness>

Use these as measurement frameworks when program loudness or true-peak estimation is relevant. They do not define the artistic loudness target for an insert spatial effect, and they do not replace exact sample/RMS/transfer measurements.

## macOS signing and notarization

- Apple notarization overview: <https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution>
- Apple code signing resources: <https://developer.apple.com/support/code-signing/>

Recheck current Xcode, certificate, hardened-runtime, entitlement, packaging, and `notarytool` requirements immediately before release. Do not encode credentials or private keys in the repository or Claude project settings.

## Audio and physical-acoustics research

For physical propagation, binaural rendering, filters, interpolation, psychoacoustics, and numerical acoustics, prefer:

- standards bodies such as ISO, IEC, ITU, AES, and EBU;
- peer-reviewed original papers;
- original algorithm papers and author-maintained implementations;
- established textbooks, with edition recorded;
- measured datasets with explicit license and acquisition conditions.

Before adopting a model, record:

- physical quantity modeled (pressure, velocity, intensity, transfer function, perception, or artistic proxy);
- domain and assumptions;
- units and coordinate convention;
- frequency, distance, time, and spatial validity range;
- numerical discretization and error;
- rendering topology;
- dataset/license constraints;
- CPU, latency, and automation consequences.

## Reference-selection checklist

- [ ] Is the source primary and current enough for this decision?
- [ ] Is the exact version/date recorded?
- [ ] Does it support the actual claim, not merely adjacent background?
- [ ] Are assumptions and invalid regimes documented?
- [ ] Has the source been mapped to the current code path and output topology?
- [ ] Is an independent test/reference implementation still needed?
- [ ] Are licensing and redistribution terms known?
- [ ] Has the relevant ADR/experiment/assumption entry been updated?
