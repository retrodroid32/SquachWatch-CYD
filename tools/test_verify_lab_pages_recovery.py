#!/usr/bin/env python3
"""Fail-closed host regressions for protected master-context Pages recovery."""
from copy import deepcopy
from pathlib import Path
import json
from verify_lab_pages_recovery import DEPLOY_JOB, RELEASE_WORKFLOW, verify

REPO = "retrodroid32/SquachWatch-CYD"
SHA = "f6309859fd70ff5655c84322b54f41d5c29a4851"
RUN = {
    "name": RELEASE_WORKFLOW, "event": "workflow_dispatch",
    "status": "completed", "conclusion": "failure", "head_sha": SHA,
    "head_branch": "lab/hunt-history-peak-graph-r1-20261111",
    "head_repository": {"full_name": REPO},
}
JOBS = {"total_count": 4, "jobs": [
    {"name": "preflight", "conclusion": "success"},
    {"name": "ci-gate", "conclusion": "success"},
    {"name": "build-and-release", "conclusion": "success"},
    {"name": DEPLOY_JOB, "conclusion": "failure"},
]}


def rejected(run, jobs, sha=SHA, repo=REPO):
    try:
        verify(run, jobs, sha, repo)
    except ValueError:
        return
    raise AssertionError("unsafe or incomplete recovery was accepted")


verify(deepcopy(RUN), deepcopy(JOBS), SHA, REPO)
for key, bad in [
    ("name", "Build"), ("event", "push"), ("status", "in_progress"),
    ("conclusion", "success"), ("head_sha", "0" * 40),
    ("head_branch", "master"), ("head_branch", "feature/unreviewed"),
    ("head_repository", {"full_name": "other/repo"}),
]:
    run = deepcopy(RUN)
    run[key] = bad
    rejected(run, deepcopy(JOBS))
rejected(deepcopy(RUN), deepcopy(JOBS), "0" * 40)
rejected(deepcopy(RUN), deepcopy(JOBS), "not-a-sha")
rejected(deepcopy(RUN), deepcopy(JOBS), SHA, "other/repo")
for i in range(3):
    jobs = deepcopy(JOBS)
    jobs["jobs"][i]["conclusion"] = "failure"
    rejected(deepcopy(RUN), jobs)
jobs = deepcopy(JOBS)
jobs["jobs"][3]["name"] = "firmware"
rejected(deepcopy(RUN), jobs)
jobs = deepcopy(JOBS)
jobs["jobs"].append({"name": "unexpected", "conclusion": "cancelled"})
jobs["total_count"] = 5
rejected(deepcopy(RUN), jobs)
jobs = deepcopy(JOBS)
jobs["total_count"] = 5
rejected(deepcopy(RUN), jobs)
# The one-time merge request must pin precisely the failed, validated HUNT
# source. A later change to this request must go through review again.
root = Path(__file__).resolve().parents[1]
request = json.loads((root / ".github/lab-pages-recover-request.json").read_text(encoding="utf-8"))
assert request == {"source_run_id": 43381302822, "expected_sha": SHA}
workflow = (root / ".github/workflows/publish-pages.yml").read_text(encoding="utf-8")
assert "paths:" in workflow and "'.github/lab-pages-recover-request.json'" in workflow
assert "github.ref == 'refs/heads/master'" in workflow
assert "VERIFIED_LAB_SOURCE_RUN_ID" in workflow
assert "workflow_dispatch:" in workflow and "workflow_run:" in workflow
print("PASS: protected LAB Pages recovery gate, strict SHA, one-shot master push")
