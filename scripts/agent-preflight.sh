#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

printf '\n== DNA Orbit agent preflight ==\n'
printf 'Root: %s\n' "$ROOT_DIR"

if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  printf 'ERROR: not inside a Git worktree.\n' >&2
  exit 1
fi

BRANCH="$(git branch --show-current 2>/dev/null || true)"
COMMIT="$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
printf 'Branch: %s\nCommit: %s\n' "${BRANCH:-detached}" "$COMMIT"

if [[ "$BRANCH" == "main" || "$BRANCH" == "master" ]]; then
  printf 'WARNING: current branch is %s. Production edits must use a feature branch.\n' "$BRANCH" >&2
fi

printf '\n-- Working tree --\n'
git status --short --branch

printf '\n-- Recent commits --\n'
git log -8 --oneline --decorate

printf '\n-- Current diff summary --\n'
git diff --stat || true
git diff --cached --stat || true

REQUIRED_FILES=(
  "AGENTS.md"
  "CLAUDE.md"
  ".claude/settings.json"
  ".claude/rules/dsp-physics.md"
  ".claude/rules/audio-quality.md"
  ".claude/skills/quality-governance/SKILL.md"
  ".claude/skills/start-dsp-task/SKILL.md"
  ".claude/skills/physics-audit/SKILL.md"
  ".claude/skills/audio-quality-gate/SKILL.md"
  ".claude/skills/adversarial-review/SKILL.md"
  ".claude/skills/release-gate/SKILL.md"
  ".claude/agents/quality-governor.md"
  "docs/governance/PRODUCT_CONSTITUTION.md"
  "docs/governance/QUALITY_GOVERNANCE_SYSTEM.md"
  "docs/governance/CHANGE_RISK_MODEL.md"
  "docs/governance/DECISION_AND_EXCEPTION_POLICY.md"
  "quality/governance.json"
  "quality/constitution.sha256"
  "docs/quality/QUALITY_DEBT_LEDGER.md"
  "docs/quality/RELEASE_DECISION_LOG.md"
  "scripts/quality-governance-audit.py"
  "scripts/quality-gate.sh"
  "docs/claude-code/README.md"
  "docs/claude-code/PHYSICS_FIDELITY_STANDARD.md"
  "docs/claude-code/AUDIO_QUALITY_STANDARD.md"
  "docs/claude-code/ASSUMPTIONS_REGISTER.md"
  "MANUAL_REQUIRED.md"
)

MISSING=0
printf '\n-- Foundation files --\n'
for path in "${REQUIRED_FILES[@]}"; do
  if [[ -f "$path" ]]; then
    printf 'OK      %s\n' "$path"
  else
    printf 'MISSING %s\n' "$path" >&2
    MISSING=1
  fi
done

printf '\n-- Constitution --\n'
if command -v shasum >/dev/null 2>&1; then
  shasum -a 256 docs/governance/PRODUCT_CONSTITUTION.md
elif command -v sha256sum >/dev/null 2>&1; then
  sha256sum docs/governance/PRODUCT_CONSTITUTION.md
else
  printf 'No shell SHA-256 tool found; Python governance audit will verify integrity.\n'
fi
printf 'Expected lock: '
cat quality/constitution.sha256 2>/dev/null || true

printf '\n-- Tool versions --\n'
printf 'git:    %s\n' "$(git --version 2>/dev/null || echo unavailable)"
printf 'python: %s\n' "$(python3 --version 2>/dev/null || echo unavailable)"
printf 'cmake:  %s\n' "$(cmake --version 2>/dev/null | head -n 1 || echo unavailable)"
printf 'c++:    %s\n' "$(c++ --version 2>/dev/null | head -n 1 || echo unavailable)"
printf 'claude: %s\n' "$(claude --version 2>/dev/null | head -n 1 || echo unavailable)"
printf 'gh:     %s\n' "$(gh --version 2>/dev/null | head -n 1 || echo unavailable)"

if command -v gh >/dev/null 2>&1; then
  printf '\n-- Pull request --\n'
  gh pr view --json number,title,state,isDraft,headRefName,baseRefName,url 2>/dev/null || \
    printf 'No current-branch PR resolved, or gh is not authenticated.\n'
fi

if [[ "$MISSING" -ne 0 ]]; then
  printf '\nERROR: required engineering/governance foundation files are missing.\n' >&2
  exit 1
fi

printf '\nPreflight complete. This is orientation, not governance/build/test evidence.\n'
printf 'Next: bash scripts/quality-gate.sh\n'
