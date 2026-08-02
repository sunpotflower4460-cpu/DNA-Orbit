# DNA Orbit Experiment Records

Store one reproducible record per significant algorithm, physical-model, quality, preset, or performance decision.

File name:

```text
YYYY-MM-DD-short-descriptive-name.md
```

Do not overwrite an old result after changing the implementation. Create a new record or append a clearly dated rerun tied to a new commit.

## Required metadata

- date and investigator;
- repository commit and dirty/clean state;
- compiler, build type, JUCE, OS, CPU, and power mode;
- DAW/format where applicable;
- sample rate, block size, and input fixture hashes;
- truth class;
- baseline and candidates;
- predeclared hypothesis and thresholds;
- measurement/listening procedure;
- raw-data or log location;
- result and uncertainty;
- adopt/reject/iterate/inconclusive decision;
- rollback and follow-up.

Use `/experiment-design <hypothesis>` to create a complete protocol. Measurement and listening results must never be invented when the experiment cannot run.

Large or copyrighted audio should not be committed without permission. Commit generation scripts, short synthetic fixtures, metadata, hashes, plots, and machine-readable summaries instead.
