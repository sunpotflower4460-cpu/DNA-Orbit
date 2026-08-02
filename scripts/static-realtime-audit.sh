#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

WARN_ONLY="${DNA_ORBIT_STATIC_AUDIT_WARN_ONLY:-0}"
FAILURES=0
WARNINGS=0

printf '\n== DNA Orbit static realtime audit ==\n'
printf 'This scanner finds suspicious text; it does not replace call-graph review, compilation, sanitizers, profiling, or host tests.\n\n'

DSP_PATHS=("Source/dsp")
PROCESSOR_PATHS=("Source/PluginProcessor.cpp" "Source/PluginProcessor.h")

report_matches () {
  local severity="$1"
  local label="$2"
  local pattern="$3"
  shift 3
  local files=("$@")
  local output

  output="$(grep -RInE --include='*.h' --include='*.cpp' "$pattern" "${files[@]}" 2>/dev/null || true)"
  if [[ -n "$output" ]]; then
    printf '%s: %s\n%s\n\n' "$severity" "$label" "$output"
    if [[ "$severity" == "FAIL" ]]; then
      FAILURES=$((FAILURES + 1))
    else
      WARNINGS=$((WARNINGS + 1))
    fi
  fi
}

# High-confidence hazards anywhere in the core DSP directory. Review every hit.
report_matches "FAIL" "heap allocation/deallocation token in core DSP" \
  '(^|[^[:alnum:]_])(new|delete|malloc|calloc|realloc|free)[[:space:](\[]' \
  "${DSP_PATHS[@]}"

report_matches "FAIL" "blocking synchronization token in core DSP" \
  '(std::mutex|std::recursive_mutex|std::shared_mutex|std::condition_variable|std::lock_guard|std::unique_lock|std::scoped_lock|\.wait[[:space:]]*\(|sleep_for|sleep_until|usleep[[:space:]]*\()' \
  "${DSP_PATHS[@]}"

report_matches "FAIL" "filesystem/network/process/logging call in core DSP" \
  '(std::filesystem|std::fstream|std::ifstream|std::ofstream|fopen[[:space:]]*\(|printf[[:space:]]*\(|fprintf[[:space:]]*\(|std::cout|std::cerr|DBG[[:space:]]*\(|Logger::|URL[[:space:](]|ChildProcess)' \
  "${DSP_PATHS[@]}"

report_matches "FAIL" "message-thread or modal UI call in core DSP" \
  '(MessageManager|callAsync|sendNotification|AlertWindow|DialogWindow|runModalLoop|Component::)' \
  "${DSP_PATHS[@]}"

# Processor files contain legitimate non-callback construction/editor code, so report as warnings.
report_matches "WARN" "allocation or UI token in processor file; verify it is unreachable from audio callbacks" \
  '(^|[^[:alnum:]_])(new|delete|malloc|calloc|realloc|free)[[:space:](\[]|make_unique|createEditor|MessageManager|callAsync|DBG[[:space:]]*\(|Logger::)' \
  "${PROCESSOR_PATHS[@]}"

report_matches "WARN" "potentially unbounded or blocking construct; inspect call path and bounds" \
  '(while[[:space:]]*\(|std::async|std::future|join[[:space:]]*\(|system[[:space:]]*\()' \
  "${DSP_PATHS[@]}" "${PROCESSOR_PATHS[@]}"

report_matches "WARN" "portable-constant or exception token requiring review" \
  '(M_PI|throw[[:space:]]|catch[[:space:]]*\(|dynamic_cast)' \
  "${DSP_PATHS[@]}" "${PROCESSOR_PATHS[@]}"

printf '%s\n' '-- Process entry points for manual call-graph review --'
grep -RInE 'processBlock[[:space:]]*\(|processBlockBypassed[[:space:]]*\(|HelixEngine::process[[:space:]]*\(' \
  Source/PluginProcessor.cpp Source/dsp 2>/dev/null || true

printf '\n-- Outstanding markers in production DSP/processor code --\n'
grep -RInE --include='*.h' --include='*.cpp' '(TODO|FIXME|HACK|XXX)' \
  Source/dsp Source/PluginProcessor.cpp Source/PluginProcessor.h 2>/dev/null || true

printf '\nSummary: %d failure group(s), %d warning group(s).\n' "$FAILURES" "$WARNINGS"

if [[ "$FAILURES" -gt 0 && "$WARN_ONLY" != "1" ]]; then
  printf 'Static audit failed. Inspect every hit and prove whether it is reachable from the audio callback.\n' >&2
  exit 1
fi

if [[ "$FAILURES" -gt 0 ]]; then
  printf 'Warning-only mode: failure groups were reported but did not stop the command.\n'
fi

printf 'Static audit complete. A clean scan is not proof of real-time safety.\n'
