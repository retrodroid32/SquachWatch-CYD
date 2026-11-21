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
# Current LAB release workflow separates firmware from Pages. A successful
# lab-branch push can supply a fully validated pages-site even when Pages
# deployment fails in its separate workflow.
CYD28_SHA = "88e6c820b84f1da0290c142fecf28297b0498fa1"
push_run = {
    **deepcopy(RUN),
    "event": "push",
    "conclusion": "success",
    "head_sha": CYD28_SHA,
    "head_branch": "lab/cyd28-sd-boot-memory-guard-r1-20261120",
}
push_jobs = {"total_count": 3, "jobs": deepcopy(JOBS["jobs"][:3])}
verify(deepcopy(push_run), deepcopy(push_jobs), CYD28_SHA, REPO)
for key, bad in [
    ("name", "Build"), ("event", "workflow_dispatch"),
    ("status", "in_progress"), ("conclusion", "failure"),
    ("head_sha", "0" * 40), ("head_branch", "master"),
    ("head_repository", {"full_name": "other/repo"}),
]:
    run = deepcopy(push_run)
    run[key] = bad
    rejected(run, deepcopy(push_jobs), CYD28_SHA)
rejected(deepcopy(push_run), deepcopy(push_jobs), "0" * 40)
for i in range(3):
    jobs = deepcopy(push_jobs)
    jobs["jobs"][i]["conclusion"] = "failure"
    rejected(deepcopy(push_run), jobs, CYD28_SHA)
jobs = deepcopy(push_jobs)
jobs["jobs"].append({"name": DEPLOY_JOB, "conclusion": "failure"})
jobs["total_count"] = 4
rejected(deepcopy(push_run), jobs, CYD28_SHA)
jobs = deepcopy(push_jobs)
jobs["total_count"] = 4
rejected(deepcopy(push_run), jobs, CYD28_SHA)

# The one-shot merge request pins the actual successful CYD28 lab release.
# A different source must be reviewed via another change to this file.
root = Path(__file__).resolve().parents[1]
request = json.loads((root / ".github/lab-pages-recover-request.json").read_text(encoding="utf-8"))
assert request == {"source_run_id": 44800291211, "expected_sha": CYD28_SHA}
workflow = (root / ".github/workflows/publish-pages.yml").read_text(encoding="utf-8")
assert "paths:" in workflow and "'.github/lab-pages-recover-request.json'" in workflow
assert "github.ref == 'refs/heads/master'" in workflow
assert "VERIFIED_LAB_SOURCE_RUN_ID" in workflow
assert "workflow_dispatch:" in workflow and "workflow_run:" in workflow
print("PASS: protected LAB Pages recovery gate, strict SHA, one-shot master push")
