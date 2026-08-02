# Optional Claude Code Automation

The repository intentionally starts with instructions, scoped rules, skills, read-only reviewers, permissions, and explicit scripts rather than aggressive automatic hooks.

## Why heavy hooks are not enabled by default

A hook runs repeatedly and deterministically. A poorly scoped hook can:

- slow every edit;
- trigger recursive or duplicate work;
- depend on unavailable local tools;
- produce false failures from simple text matching;
- interrupt exploratory work before a coherent change exists;
- behave differently across Claude Code versions;
- encourage agents to satisfy a check rather than understand the DSP.

The current default therefore uses:

- `.claude/settings.json` for destructive-command and secret-file protection;
- `scripts/agent-preflight.sh` for explicit repository orientation;
- `scripts/static-realtime-audit.sh` for explicit callback-risk scanning;
- `scripts/validate-local.sh` for the full local validation sequence;
- `CLAUDE.md` and skills to require these checks at the correct stage.

## When to add a hook

Add a project hook only when all are true:

1. the rule is deterministic and objective;
2. the command is fast enough for its event frequency;
3. false positives are rare and actionable;
4. it works on the supported development machines;
5. failure output tells the agent exactly what to do;
6. the same rule cannot be enforced more reliably by compiler, test, linter, or permission;
7. the installed Claude Code hook schema has been checked against current official documentation.

## Good candidates

### After edits to DSP files

Run a fast warning-only scan when files under `Source/dsp/` or the processor change:

```sh
DNA_ORBIT_STATIC_AUDIT_WARN_ONLY=1 bash scripts/static-realtime-audit.sh
```

Do not run a full CMake build after every edit. The explicit validation skill already runs the expensive checks at coherent milestones.

### Before stopping after implementation

A Stop hook may remind the agent to run the release gate when production DSP files changed. It should not falsely claim the checks passed, and it should allow an intentional incomplete exploration to stop.

### Protect generated or credential paths

Use permission rules rather than hooks where possible. `.claude/settings.json` already denies common secret paths and destructive Git commands.

## Poor candidates

Avoid automatic hooks that:

- rewrite C++ or Markdown after every tool call;
- auto-commit or auto-push;
- run formatters over unrelated files;
- download dependencies or access the network silently;
- run long validators after every edit;
- use a language model to approve the same model’s changes automatically;
- block on keywords such as `new`, `lock`, or `printf` without call-path analysis;
- expose local paths, credentials, signing identities, or private audio corpus metadata.

## Suggested rollout

### Stage 0 — current default

Use explicit scripts and skills. Collect where agents forget steps or where repeated defects occur.

### Stage 1 — warning-only

Add a scoped fast hook that reports likely real-time hazards without blocking. Review false positives for several tasks.

### Stage 2 — blocking high-confidence violations

Block only proven patterns such as direct filesystem/logging calls newly added inside the audio processing call graph, after the scanner is reliable.

### Stage 3 — release workflow integration

Optionally add a Stop or manual command workflow that gathers validation output into a release evidence directory. Do not auto-sign, notarize, merge, tag, or distribute.

## Version-sensitive example process

Before editing hook configuration:

1. read the current Claude Code hooks and settings documentation;
2. inspect the installed `claude --version`;
3. place the proposed hook in a temporary local settings file first;
4. test matching, quoting, exit behavior, and timeout;
5. verify that failure does not corrupt editor or Git state;
6. document the exact version and rollback;
7. only then commit the project hook.

## Better than hooks

Prefer these durable mechanisms when applicable:

- `static_assert` and compile-time checks;
- C++ types that encode units or states;
- unit/property/regression tests;
- CMake target structure;
- sanitizer builds;
- validators;
- permissions;
- schema fixtures;
- benchmark tools;
- ADRs and assumptions register.

Automation should amplify engineering understanding, not replace it.
