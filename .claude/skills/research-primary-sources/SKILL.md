---
name: research-primary-sources
description: Research current primary sources for a DNA Orbit API, physical-acoustics, psychoacoustics, numerical method, standard, validator, signing, licensing, or toolchain decision. Use whenever a material fact is uncertain, version-sensitive, niche, or externally defined.
argument-hint: "<decision or question>"
---

# Primary-source research

Question or decision:

`$ARGUMENTS`

Read `docs/claude-code/REFERENCE_SOURCES.md` first.

## Research protocol

1. Translate the question into specific claims that need support.
2. Search official documentation, standards bodies, original papers, framework source, original algorithm papers, or authoritative datasets first.
3. Record publication/version/date and access date.
4. Distinguish direct source statements from your inference.
5. Capture assumptions, validity range, units, coordinate/timing convention, and licensing.
6. Search for conflicting primary sources or known implementation limitations.
7. Map the result to the exact DNA Orbit code path, output topology, host behavior, and product claim.
8. Identify what still needs measurement or experiment; documentation alone is not proof that the implementation behaves correctly.

## Required output

Create or update the relevant ADR, assumption entry, or experiment plan with:

- question;
- primary sources;
- claim supported by each source;
- version/date;
- direct fact versus inference;
- assumptions/invalid regimes;
- conflicts/uncertainties;
- licensing/data constraints;
- implementation consequence;
- required tests and listening;
- recommendation and confidence.

Do not cite competitor marketing or a secondary tutorial as the sole basis for a physical, API, compatibility, or release decision. Do not copy long copyrighted passages.
