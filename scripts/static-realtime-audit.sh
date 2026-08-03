#!/usr/bin/env bash
#
# Static scan for constructs that must never run on the audio thread:
# heap allocation, locking, filesystem/logging, and message-thread calls.
#
# What this is: a cheap regression tripwire. The DSP in this project is
# written to be allocation- and lock-free on the audio path, and that
# property is easy to break accidentally in a future edit. This catches the
# obvious ways.
#
# What this is NOT: proof of real-time safety. It does not follow the call
# graph, so it cannot tell whether a flagged construct is actually reachable
# from a callback, nor can it see a hazard hidden behind a function it does
# not scan. A clean scan means "no obvious new hazard", nothing stronger.
# The real guarantees come from design discipline, the sanitizer runs, and
# the tests - see MANUAL_REQUIRED.md.
#
# Usage:
#   ./scripts/static-realtime-audit.sh
#   DNA_ORBIT_STATIC_AUDIT_WARN_ONLY=1 ./scripts/static-realtime-audit.sh
#
# Exits non-zero if any FAIL group matches, unless WARN_ONLY=1.

set -uo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

WARN_ONLY="${DNA_ORBIT_STATIC_AUDIT_WARN_ONLY:-0}"
FAILURES=0
WARNINGS=0

# Core DSP: nothing here may allocate, lock, or touch the outside world.
DSP_PATHS=("Source/dsp")
# Processor: legitimately contains editor construction and other
# non-callback code, so hits here are warnings to eyeball, not failures.
PROCESSOR_PATHS=("Source/PluginProcessor.cpp" "Source/PluginProcessor.h")

# Blanks out // and /* */ comment content while preserving line numbering,
# so prose can never trip the scanner. This project comments heavily and
# deliberately ("HelixEngine stays free of...", "lock-free", "JUCE-free
# math helpers"), and the version of this scan inherited from the parallel
# branch reported every one of those as a heap-allocation FAILURE - a
# scanner that cries wolf is a scanner that gets switched off.
#
# Known limitation: a // or /* inside a string literal is treated as the
# start of a comment. There are no such literals in the scanned paths today
# (checked), and the patterns below additionally require call/declaration
# syntax rather than bare words, so a mis-strip cannot quietly hide a real
# allocation on its own.
strip_comments () {
  awk '
    BEGIN { inblock = 0 }
    {
      line = $0
      out = ""
      i = 1
      len = length(line)
      while (i <= len) {
        pair = substr(line, i, 2)
        if (inblock) {
          if (pair == "*/") { inblock = 0; i += 2 } else { i += 1 }
        } else {
          if (pair == "/*") { inblock = 1; i += 2 }
          else if (pair == "//") { break }
          else { out = out substr(line, i, 1); i += 1 }
        }
      }
      print out
    }
  ' "$1"
}

collect_sources () {
  local path
  for path in "$@"; do
    if [[ -d "$path" ]]; then
      find "$path" -type f \( -name '*.h' -o -name '*.cpp' \)
    elif [[ -f "$path" ]]; then
      printf '%s\n' "$path"
    fi
  done
}

report_matches () {
  local severity="$1"
  local label="$2"
  local pattern="$3"
  shift 3

  local output=""
  local file hits
  while IFS= read -r file; do
    [[ -n "$file" ]] || continue
    hits="$(strip_comments "$file" | grep -nE "$pattern" 2>/dev/null || true)"
    if [[ -n "$hits" ]]; then
      output+="$(printf '%s\n' "$hits" | sed "s|^|${file}:|")"$'\n'
    fi
  done < <(collect_sources "$@")

  if [[ -n "$output" ]]; then
    printf '%s: %s\n%s\n' "$severity" "$label" "$output"
    if [[ "$severity" == "FAIL" ]]; then
      FAILURES=$((FAILURES + 1))
    else
      WARNINGS=$((WARNINGS + 1))
    fi
  fi
}

printf '\n== DNA Orbit static realtime audit ==\n'
printf 'Comment-stripped token scan. Not a substitute for call-graph review,\n'
printf 'sanitizers, profiling, or host testing - see the header of this script.\n\n'

# Allocation: require call syntax (malloc(...)) or a typed new/delete, so
# prose and identifiers containing these words cannot match even if the
# comment stripper is defeated by an unusual string literal.
report_matches "FAIL" "heap allocation/deallocation in core DSP" \
  '(^|[^[:alnum:]_.>])(malloc|calloc|realloc|free)[[:space:]]*\(|(^|[^[:alnum:]_])(new|delete)[[:space:]]+[A-Za-z_:]|(^|[^[:alnum:]_])delete[[:space:]]*\[' \
  "${DSP_PATHS[@]}"

report_matches "FAIL" "blocking synchronization in core DSP" \
  '(std::(mutex|recursive_mutex|shared_mutex|condition_variable|lock_guard|unique_lock|scoped_lock))|(\.wait[[:space:]]*\()|((sleep_for|sleep_until|usleep)[[:space:]]*\()' \
  "${DSP_PATHS[@]}"

report_matches "FAIL" "filesystem/network/process/logging call in core DSP" \
  '(std::(filesystem|fstream|ifstream|ofstream|cout|cerr))|((fopen|printf|fprintf)[[:space:]]*\()|(DBG[[:space:]]*\()|(Logger::)|(ChildProcess)' \
  "${DSP_PATHS[@]}"

report_matches "FAIL" "message-thread or modal UI call in core DSP" \
  '(MessageManager|callAsync|sendNotification|AlertWindow|DialogWindow|runModalLoop)' \
  "${DSP_PATHS[@]}"

# Not automatically wrong, but each can hide an allocation or an indirect
# call the reader did not intend, so they are worth a periodic look.
report_matches "WARN" "indirection that can hide an allocation; confirm the audio path stays direct" \
  '(std::function|std::shared_ptr|std::make_shared|std::vector|std::string)' \
  "${DSP_PATHS[@]}"

report_matches "WARN" "allocation or UI token in processor file; verify it is unreachable from audio callbacks" \
  '((^|[^[:alnum:]_.>])(malloc|calloc|realloc|free)[[:space:]]*\()|((^|[^[:alnum:]_])(new|delete)[[:space:]]+[A-Za-z_:])|make_unique|createEditor|MessageManager|callAsync|(DBG[[:space:]]*\()|Logger::' \
  "${PROCESSOR_PATHS[@]}"

report_matches "WARN" "unbounded or blocking construct; inspect call path and bounds" \
  '(std::async|std::future)|((join|system)[[:space:]]*\()' \
  "${DSP_PATHS[@]}" "${PROCESSOR_PATHS[@]}"

# M_PI is not standard C++ (MSVC only defines it with _USE_MATH_DEFINES);
# this project removed it deliberately in favour of orbitmath::pi, so a
# reappearance is a portability regression.
report_matches "WARN" "non-portable constant or exception token requiring review" \
  '(^|[^[:alnum:]_])M_PI([^[:alnum:]_]|$)|(^|[^[:alnum:]_])(throw|catch)[[:space:]]*[({]|dynamic_cast' \
  "${DSP_PATHS[@]}" "${PROCESSOR_PATHS[@]}"

printf -- '-- Audio-thread entry points, for manual call-graph review --\n'
grep -RInE 'void[[:space:]]+[A-Za-z_]+::processBlock(Bypassed)?[[:space:]]*\(|HelixEngine::process[[:space:]]*\(' \
  Source/PluginProcessor.cpp Source/dsp 2>/dev/null || true

printf '\n-- Outstanding markers in production DSP/processor code --\n'
while IFS= read -r file; do
  [[ -n "$file" ]] || continue
  grep -nE '(TODO|FIXME|HACK|XXX)' "$file" 2>/dev/null | sed "s|^|${file}:|" || true
done < <(collect_sources "${DSP_PATHS[@]}" "${PROCESSOR_PATHS[@]}")

printf '\nSummary: %d failure group(s), %d warning group(s).\n' "$FAILURES" "$WARNINGS"

if [[ "$FAILURES" -gt 0 && "$WARN_ONLY" != "1" ]]; then
  printf 'Static audit FAILED. Inspect every hit and prove whether it is reachable from the audio callback.\n' >&2
  exit 1
fi

if [[ "$FAILURES" -gt 0 ]]; then
  printf 'Warning-only mode: failure groups were reported but did not stop the command.\n'
fi

printf 'Static audit complete. A clean scan is not proof of real-time safety.\n'
