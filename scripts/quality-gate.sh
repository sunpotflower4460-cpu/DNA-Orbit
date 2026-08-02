#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

MODE_ARGS=()
if [[ "${1:-}" == "--release" ]]; then
  MODE_ARGS+=("--release")
  shift
fi

OUTPUT="${DNA_ORBIT_GOVERNANCE_REPORT:-build-local-release/governance-report.json}"

python3 scripts/quality-governance-audit.py --self-test
python3 scripts/quality-governance-audit.py \
  "${MODE_ARGS[@]}" \
  --output "$OUTPUT" \
  "$@"

printf 'Governance report: %s\n' "$OUTPUT"
