#!/usr/bin/env bash
#
# Local equivalent of the CI matrix's Linux lane (docs/commercial-upgrade/
# 04_QA_商用リリース仕様書.md §2), since this project deliberately runs no
# GitHub Actions / CI service (see MANUAL_REQUIRED.md's "## CI" section).
# Run this before pushing anything that touches Source/ or Tests/:
#
#   ./Tools/local_validate.sh
#
# Steps, each of which must pass for the script to exit 0:
#   1. Release build + full CTest suite
#   2. ASan+UBSan Debug build + full CTest suite (GCC/Clang only)
#   3. This project's own sources (not JUCE's) rebuilt with -Werror
#   4. clang-tidy against this project's own .cpp files (see .clang-tidy;
#      skipped with a warning if clang-tidy is not installed)
#
# Every build directory this script creates is removed afterward (success
# or failure) - none of them are meant to persist, matching the existing
# ASan-build convention already used throughout this project's history.

set -uo pipefail

cd "$(dirname "$0")/.."

FAILED=0
STEP=0

step() {
    STEP=$((STEP + 1))
    echo
    echo "=== [$STEP] $1 ==="
}

fail() {
    echo "*** FAILED: $1 ***"
    FAILED=1
}

cleanup() {
    rm -rf build-validate-release build-validate-asan build-validate-werror
}
trap cleanup EXIT

# --- 1. Release build + CTest ------------------------------------------------
step "Release build + full CTest suite"
if cmake -S . -B build-validate-release -DCMAKE_BUILD_TYPE=Release >/tmp/local_validate_release_configure.log 2>&1 \
    && cmake --build build-validate-release -j >/tmp/local_validate_release_build.log 2>&1 \
    && ctest --test-dir build-validate-release --output-on-failure >/tmp/local_validate_release_ctest.log 2>&1
then
    echo "OK - see /tmp/local_validate_release_ctest.log for the full test list"
else
    fail "Release build/CTest (logs: /tmp/local_validate_release_{configure,build,ctest}.log)"
fi

# --- 2. ASan+UBSan ------------------------------------------------------------
step "ASan+UBSan Debug build + full CTest suite"
if cmake -S . -B build-validate-asan -DCMAKE_BUILD_TYPE=Debug -DDNA_ORBIT_SANITIZERS="address,undefined" >/tmp/local_validate_asan_configure.log 2>&1 \
    && cmake --build build-validate-asan -j >/tmp/local_validate_asan_build.log 2>&1 \
    && ctest --test-dir build-validate-asan --output-on-failure >/tmp/local_validate_asan_ctest.log 2>&1
then
    echo "OK - see /tmp/local_validate_asan_ctest.log for the full test list"
else
    fail "ASan+UBSan build/CTest (logs: /tmp/local_validate_asan_{configure,build,ctest}.log)"
fi

# --- 3. Warnings-as-errors (this project's own sources only) -----------------
step "This project's own sources, rebuilt with -Werror"
if cmake -S . -B build-validate-werror -DCMAKE_BUILD_TYPE=Release -DDNA_ORBIT_WARNINGS_AS_ERRORS=ON >/tmp/local_validate_werror_configure.log 2>&1 \
    && cmake --build build-validate-werror --target DNAOrbit DNAOrbitTests -j >/tmp/local_validate_werror_build.log 2>&1
then
    echo "OK"
else
    fail "-Werror build (logs: /tmp/local_validate_werror_{configure,build}.log)"
fi

# --- 4. clang-tidy -------------------------------------------------------------
step "clang-tidy (this project's own .cpp files; see .clang-tidy)"
if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "SKIPPED - clang-tidy is not installed"
else
    # Reuses the Release build's compile_commands.json (CMAKE_EXPORT_COMPILE_COMMANDS
    # is on project-wide), so this must run after step 1.
    SOURCES="Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/dsp/HelixEngine.cpp Source/ui/HelixView3D.cpp Source/ui/DnaLookAndFeel.cpp"
    if clang-tidy -p build-validate-release $SOURCES >/tmp/local_validate_clang_tidy.log 2>&1; then
        if grep -qE "warning:|error:" /tmp/local_validate_clang_tidy.log; then
            fail "clang-tidy found findings (log: /tmp/local_validate_clang_tidy.log)"
        else
            echo "OK"
        fi
    else
        fail "clang-tidy itself failed to run (log: /tmp/local_validate_clang_tidy.log)"
    fi
fi

echo
if [ "$FAILED" -eq 0 ]; then
    echo "=== local_validate.sh: ALL CHECKS PASSED ==="
    exit 0
else
    echo "=== local_validate.sh: ONE OR MORE CHECKS FAILED - see messages above ==="
    exit 1
fi
