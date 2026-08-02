# Local validation scripts

GitHub Actions is intentionally not used for this project. Run the commercial
validation sequence on the development Mac with:

```sh
bash scripts/validate-local.sh
```

The script performs:

1. Release configure and build;
2. complete CTest suite;
3. Debug ASan + UBSan build and CTest on macOS/Linux;
4. deterministic DSP benchmark;
5. writes benchmark output to `build-local-release/dsp-benchmark.txt`.

Optional environment variables:

```sh
DNA_ORBIT_JOBS=8 \
DNA_ORBIT_BENCHMARK_SECONDS=5 \
DNA_ORBIT_RELEASE_BUILD=build-release \
DNA_ORBIT_SANITIZER_BUILD=build-asan \
bash scripts/validate-local.sh
```

The script stops on the first failure. Do not change test tolerances merely to
make it finish; inspect whether the implementation or test assumption is wrong.

After it passes, the remaining gates are manual:

- inspect `DNAOrbitRenderShots` images at all editor sizes;
- run pluginval strictness 10;
- run Steinberg VST3 Validator;
- run `auval` on macOS;
- scan/load/automate/save/reopen/bounce in real DAWs;
- complete the listening matrix in `MANUAL_REQUIRED.md`;
- sign, notarise and package release artifacts.

The contents API does not preserve executable bits reliably, so invoke the
script through `bash` as shown above. Running `chmod +x scripts/validate-local.sh`
is optional.
