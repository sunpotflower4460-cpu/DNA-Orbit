#!/usr/bin/env python3
"""Deterministic, standard-library-only governance audit for DNA Orbit.

This tool checks decision-system integrity. It does not claim to prove sound
quality, usability, physical accuracy, or host compatibility.
"""

from __future__ import annotations

import argparse
import datetime as dt
import fnmatch
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
from typing import Iterable


RISK_VALUE = {"R0": 0, "R1": 1, "R2": 2, "R3": 3, "R4": 4}


class Audit:
    def __init__(self) -> None:
        self.blockers: list[str] = []
        self.warnings: list[str] = []
        self.info: list[str] = []

    def block(self, message: str) -> None:
        self.blockers.append(message)

    def warn(self, message: str) -> None:
        self.warnings.append(message)

    def note(self, message: str) -> None:
        self.info.append(message)


def run_git(root: Path, *args: str) -> tuple[int, str]:
    try:
        process = subprocess.run(
            ["git", *args],
            cwd=root,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            check=False,
        )
    except OSError:
        return 127, ""
    return process.returncode, process.stdout.strip()


def git_ref_exists(root: Path, ref: str) -> bool:
    code, _ = run_git(root, "rev-parse", "--verify", "--quiet", ref)
    return code == 0


def resolve_base(root: Path, requested: str | None) -> str | None:
    candidates = [requested] if requested else []
    candidates.extend([os.environ.get("DNA_ORBIT_GOVERNANCE_BASE"), "origin/main", "main", "HEAD^"])
    for candidate in candidates:
        if candidate and git_ref_exists(root, candidate):
            return candidate
    return None


def changed_files(root: Path, base: str | None, audit: Audit) -> set[str]:
    changed: set[str] = set()
    code, _ = run_git(root, "rev-parse", "--is-inside-work-tree")
    if code != 0:
        audit.warn("Git worktree unavailable; changed-path risk classification was skipped.")
        return changed

    if base:
        code, merge_base = run_git(root, "merge-base", base, "HEAD")
        if code == 0 and merge_base:
            code, output = run_git(root, "diff", "--name-only", f"{merge_base}...HEAD")
            if code == 0:
                changed.update(line for line in output.splitlines() if line)
        else:
            audit.warn(f"Could not calculate merge base against {base}.")
    else:
        audit.warn("No base ref could be resolved; committed change classification was skipped.")

    for args in (("diff", "--name-only"), ("diff", "--cached", "--name-only")):
        code, output = run_git(root, *args)
        if code == 0:
            changed.update(line for line in output.splitlines() if line)

    return {Path(path).as_posix() for path in changed}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def matches(path: str, patterns: Iterable[str]) -> bool:
    return any(fnmatch.fnmatchcase(path, pattern) for pattern in patterns)


def classify_path(path: str, rules: list[dict]) -> str:
    best = "R0"
    best_value = -1
    for rule in rules:
        tier = rule.get("tier", "R0")
        if tier not in RISK_VALUE:
            continue
        if matches(path, rule.get("patterns", [])) and RISK_VALUE[tier] > best_value:
            best = tier
            best_value = RISK_VALUE[tier]
    return best


def highest_risk(paths: Iterable[str], rules: list[dict]) -> tuple[str, dict[str, list[str]]]:
    grouped = {tier: [] for tier in RISK_VALUE}
    highest = "R0"
    for path in sorted(paths):
        tier = classify_path(path, rules)
        grouped[tier].append(path)
        if RISK_VALUE[tier] > RISK_VALUE[highest]:
            highest = tier
    return highest, grouped


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def parse_field(text: str, name: str) -> str | None:
    pattern = re.compile(rf"^\s*-?\s*{re.escape(name)}\s*:\s*(.*?)\s*$", re.IGNORECASE | re.MULTILINE)
    match = pattern.search(text)
    return match.group(1).strip() if match else None


def split_patterns(raw: str | None) -> list[str]:
    if not raw:
        return []
    return [part.strip() for part in re.split(r"[;,]", raw) if part.strip()]


def parse_risk(raw: str | None) -> str | None:
    if not raw:
        return None
    match = re.search(r"\bR[0-4]\b", raw.upper())
    return match.group(0) if match else None


def validate_headings(path: Path, headings: list[str], audit: Audit) -> str:
    text = read_text(path)
    for heading in headings:
        if heading not in text:
            audit.block(f"Change record {path} is missing required heading: {heading}")
    placeholders = ("<short outcome>", "CHG-YYYYMMDD", "Risk tier: R0 /", "Covered paths: <")
    for placeholder in placeholders:
        if placeholder in text:
            audit.block(f"Change record {path} still contains template placeholder: {placeholder}")
    return text


def audit_exceptions(root: Path, policy: dict, release: bool, audit: Audit) -> list[dict]:
    directory = root / policy["exceptions"]["directory"]
    today = dt.date.today()
    active: list[dict] = []
    if not directory.exists():
        audit.block(f"Exception directory missing: {directory.relative_to(root)}")
        return active

    for path in sorted(directory.glob("*.md")):
        if path.name.upper() in {"TEMPLATE.MD", "README.MD"}:
            continue
        text = read_text(path)
        status = (parse_field(text, "Status") or "").lower()
        if status not in {"proposed", "active", "resolved", "revoked"}:
            audit.block(f"Exception {path.relative_to(root)} has invalid or missing Status.")
            continue
        if status != "active":
            continue

        owner = parse_field(text, "Owner")
        expiry_raw = parse_field(text, "Expiry")
        approved_by = parse_field(text, "Approved by")
        if not owner:
            audit.block(f"Active exception {path.relative_to(root)} has no owner.")
        if not expiry_raw:
            audit.block(f"Active exception {path.relative_to(root)} has no expiry date.")
            expiry = None
        else:
            try:
                expiry = dt.date.fromisoformat(expiry_raw)
            except ValueError:
                audit.block(f"Active exception {path.relative_to(root)} has invalid expiry: {expiry_raw}")
                expiry = None
        if expiry and expiry < today:
            audit.block(f"Active exception {path.relative_to(root)} expired on {expiry}.")
        if release and policy["exceptions"].get("release_blocks_active_unapproved", True):
            if not approved_by or approved_by.lower() in {"pending", "none", "tbd", "—"}:
                audit.block(f"Release mode: active exception {path.relative_to(root)} lacks approval.")
        active.append({"path": path.relative_to(root).as_posix(), "owner": owner, "expiry": expiry_raw})
    return active


def changed_records(root: Path, changed: set[str], directory: str, template_name: str = "TEMPLATE.md") -> list[Path]:
    prefix = directory.rstrip("/") + "/"
    return [
        root / path
        for path in sorted(changed)
        if path.startswith(prefix) and Path(path).name != template_name and path.endswith(".md")
    ]


def self_test() -> int:
    rules = [
        {"tier": "R4", "patterns": ["Source/Parameters.h"]},
        {"tier": "R3", "patterns": ["Source/dsp/*"]},
        {"tier": "R2", "patterns": ["Source/ui/*"]},
        {"tier": "R0", "patterns": ["*.md"]},
    ]
    cases = {
        "Source/Parameters.h": "R4",
        "Source/dsp/HelixEngine.cpp": "R3",
        "Source/ui/View.cpp": "R2",
        "README.md": "R0",
    }
    failures = [
        f"{path}: expected {expected}, got {classify_path(path, rules)}"
        for path, expected in cases.items()
        if classify_path(path, rules) != expected
    ]
    if split_patterns("Source/dsp/*; Tests/*, Source/Parameters.h") != [
        "Source/dsp/*", "Tests/*", "Source/Parameters.h"
    ]:
        failures.append("coverage pattern parsing failed")
    if parse_risk("R4 / CONSTITUTIONAL") != "R4":
        failures.append("risk parsing failed")
    try:
        assert dt.date.fromisoformat("2026-08-02") == dt.date(2026, 8, 2)
    except (AssertionError, ValueError) as exc:
        failures.append(f"date parsing failed: {exc}")
    if failures:
        print("Governance auditor self-test FAILED", file=sys.stderr)
        for failure in failures:
            print(f"- {failure}", file=sys.stderr)
        return 1
    print("Governance auditor self-test PASSED")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", default=None, help="repository root; defaults to script parent")
    parser.add_argument("--base", default=None, help="Git base ref for changed-path classification")
    parser.add_argument("--release", action="store_true", help="enable release-authorisation strictness")
    parser.add_argument("--output", default=None, help="write JSON report to this path")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        return self_test()

    root = Path(args.root).resolve() if args.root else Path(__file__).resolve().parent.parent
    audit = Audit()
    manifest_path = root / "quality/governance.json"
    if not manifest_path.exists():
        print("BLOCKER: quality/governance.json is missing", file=sys.stderr)
        return 1

    try:
        policy = json.loads(read_text(manifest_path))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"BLOCKER: cannot read governance manifest: {exc}", file=sys.stderr)
        return 1

    for relative in policy.get("required_files", []):
        if not (root / relative).is_file():
            audit.block(f"Required governance artifact missing: {relative}")

    constitution = policy["constitution"]
    constitution_path = root / constitution["path"]
    if constitution_path.is_file():
        actual_hash = sha256(constitution_path)
        expected_hash = constitution["expected_sha256"]
        if actual_hash != expected_hash:
            audit.block(
                f"Product Constitution hash mismatch: expected {expected_hash}, got {actual_hash}. "
                "Use the constitutional amendment process; do not update the lock casually."
            )
        else:
            audit.note(f"Constitution integrity verified: {actual_hash}")

        lock_path = root / constitution["sha256_file"]
        if lock_path.is_file() and expected_hash not in read_text(lock_path):
            audit.block(f"Constitution lock file does not contain expected hash: {lock_path.relative_to(root)}")
    else:
        audit.block(f"Product Constitution missing: {constitution['path']}")

    base = resolve_base(root, args.base)
    changed = changed_files(root, base, audit)
    rules = policy.get("risk_rules", [])
    risk, grouped = highest_risk(changed, rules)
    audit.note(f"Resolved base: {base or 'none'}")
    audit.note(f"Changed files considered: {len(changed)}")
    audit.note(f"Highest change risk: {risk}")

    record_policy = policy["change_record"]
    records = changed_records(root, changed, record_policy["directory"])
    threshold = policy.get("change_record_required_at_or_above", "R2")
    governed_changed = {
        path for path in changed
        if RISK_VALUE[classify_path(path, rules)] >= RISK_VALUE[threshold]
        and not path.startswith(record_policy["directory"].rstrip("/") + "/")
    }

    if governed_changed and not records:
        audit.block(f"{risk} change requires a changed record under {record_policy['directory']}/")

    record_reports: list[dict] = []
    covered_by_any: set[str] = set()
    for record in records:
        if not record.is_file():
            audit.block(f"Changed record cannot be read: {record.relative_to(root)}")
            continue
        text = validate_headings(record, record_policy["required_headings"], audit)
        declared_risk = parse_risk(parse_field(text, "Risk tier"))
        coverage_patterns = split_patterns(parse_field(text, "Covered paths"))
        if declared_risk is None:
            audit.block(f"Change record {record.relative_to(root)} has no valid Risk tier field.")
            declared_risk = "R0"
        if not coverage_patterns:
            audit.block(f"Change record {record.relative_to(root)} has no Covered paths patterns.")

        covered = sorted(path for path in governed_changed if matches(path, coverage_patterns))
        covered_by_any.update(covered)
        actual_risk, _ = highest_risk(covered, rules)
        if covered and RISK_VALUE[declared_risk] < RISK_VALUE[actual_risk]:
            audit.block(
                f"Change record {record.relative_to(root)} declares {declared_risk} "
                f"but covers {actual_risk} paths."
            )
        if not covered:
            audit.warn(f"Changed record {record.relative_to(root)} covers no R2+ changed path in this diff.")
        record_reports.append({
            "path": record.relative_to(root).as_posix(),
            "declared_risk": declared_risk,
            "coverage_patterns": coverage_patterns,
            "covered_changed_paths": covered,
            "actual_covered_risk": actual_risk,
        })

    uncovered = sorted(governed_changed - covered_by_any)
    for path in uncovered:
        audit.block(f"R2+ changed path is not covered by any changed change record: {path}")

    critical = policy["critical_change_requirements"]
    if changed and RISK_VALUE[risk] >= RISK_VALUE[policy.get("adr_required_at_or_above", "R3")]:
        adrs = changed_records(root, changed, critical["adr_directory"], template_name="")
        if not adrs:
            audit.block(f"{risk} change requires a changed ADR under {critical['adr_directory']}/")
    else:
        adrs = []
    if changed and RISK_VALUE[risk] >= RISK_VALUE[policy.get("independent_review_required_at_or_above", "R3")]:
        reviews = changed_records(root, changed, critical["review_directory"])
        if not reviews:
            audit.block(f"{risk} change requires a changed independent review under {critical['review_directory']}/")
    else:
        reviews = []

    constitution_changed = constitution["path"] in changed
    if constitution_changed:
        for required_changed in (constitution["sha256_file"], "quality/governance.json"):
            if required_changed not in changed:
                audit.block(f"Constitution changed without updating {required_changed}.")
        marker = critical.get("constitution_owner_approval_marker", "Owner approval: APPROVED")
        owner_approved = any(marker in read_text(path) for path in reviews if path.is_file())
        if args.release and not owner_approved:
            audit.block("Release mode: constitutional change lacks recorded product-owner approval.")
        elif not owner_approved:
            audit.warn("Constitution changed; product-owner approval remains a manual release gate.")

    active_exceptions = audit_exceptions(root, policy, args.release, audit)

    report = {
        "schema_version": 1,
        "mode": "release" if args.release else "development",
        "root": str(root),
        "base": base,
        "highest_risk": risk,
        "changed_files": sorted(changed),
        "changed_files_by_risk": grouped,
        "governed_changed_paths": sorted(governed_changed),
        "uncovered_governed_paths": uncovered,
        "change_records": record_reports,
        "adr_records": [path.relative_to(root).as_posix() for path in adrs],
        "review_records": [path.relative_to(root).as_posix() for path in reviews],
        "active_exceptions": active_exceptions,
        "blockers": audit.blockers,
        "warnings": audit.warnings,
        "info": audit.info,
        "verdict": "FAIL" if audit.blockers else "PASS_WITH_WARNINGS" if audit.warnings else "PASS",
    }

    if args.output:
        output = Path(args.output)
        if not output.is_absolute():
            output = root / output
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")

    print("\n== DNA Orbit quality governance audit ==")
    print(f"Mode: {report['mode']}")
    print(f"Base: {base or 'unresolved'}")
    print(f"Highest risk: {risk}")
    print(f"R2+ paths: {len(governed_changed)}; uncovered: {len(uncovered)}")
    for message in audit.info:
        print(f"INFO: {message}")
    for message in audit.warnings:
        print(f"WARNING: {message}")
    for message in audit.blockers:
        print(f"BLOCKER: {message}", file=sys.stderr)
    print(f"Verdict: {report['verdict']}")
    return 1 if audit.blockers else 0


if __name__ == "__main__":
    raise SystemExit(main())
