#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

JOBS="${DNA_ORBIT_JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
RELEASE_BUILD="${DNA_ORBIT_RELEASE_BUILD:-build-local-release}"
SANITIZER_BUILD="${DNA_ORBIT_SANITIZER_BUILD:-build-local-asan}"
BENCHMARK_SECONDS="${DNA_ORBIT_BENCHMARK_SECONDS:-2}"

printf '\n== DNA Orbit local validation ==\n'
printf 'Root: %s\nJobs: %s\n\n' "$ROOT_DIR" "$JOBS"

printf '== 1/5 Release configure ==\n'
cmake -S . -B "$RELEASE_BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DDNA_ORBIT_BUILD_TESTS=ON \
  -DDNA_ORBIT_BUILD_TOOLS=ON \
  -DDNA_ORBIT_BUILD_BENCHMARKS=ON

printf '\n== 2/5 Release build ==\n'
cmake --build "$RELEASE_BUILD" --config Release -j "$JOBS"

printf '\n== 3/5 Unit tests ==\n'
ctest --test-dir "$RELEASE_BUILD" -C Release --output-on-failure

printf '\n== 4/5 Sanitizer configure/build/test ==\n'
if [[ "$(uname -s)" == "Darwin" || "$(uname -s)" == "Linux" ]]; then
  cmake -S . -B "$SANITIZER_BUILD" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DDNA_ORBIT_BUILD_TESTS=ON \
    -DDNA_ORBIT_BUILD_TOOLS=OFF \
    -DDNA_ORBIT_BUILD_BENCHMARKS=OFF \
    -DDNA_ORBIT_SANITIZERS="address,undefined"
  cmake --build "$SANITIZER_BUILD" --config Debug -j "$JOBS"
  ctest --test-dir "$SANITIZER_BUILD" -C Debug --output-on-failure
else
  printf 'Sanitizer step skipped on this platform.\n'
fi

printf '\n== 5/5 DSP benchmark ==\n'
BENCHMARK_CANDIDATES=(
  "$RELEASE_BUILD/DNAOrbitBenchmark"
  "$RELEASE_BUILD/Release/DNAOrbitBenchmark"
  "$RELEASE_BUILD/DNAOrbitBenchmark.exe"
  "$RELEASE_BUILD/Release/DNAOrbitBenchmark.exe"
)

BENCHMARK=""
for candidate in "${BENCHMARK_CANDIDATES[@]}"; do
  if [[ -x "$candidate" ]]; then
    BENCHMARK="$candidate"
    break
  fi
done

if [[ -n "$BENCHMARK" ]]; then
  "$BENCHMARK" "$BENCHMARK_SECONDS" | tee "$RELEASE_BUILD/dsp-benchmark.txt"
else
  printf 'Benchmark executable was not found. Inspect the build output before release.\n' >&2
  exit 1
fi

printf '\nValidation completed.\n'
printf 'Next manual gates: RenderShots inspection, pluginval/VST3 Validator/auval, DAW tests and listening.\n'
