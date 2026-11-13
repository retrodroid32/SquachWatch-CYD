#!/usr/bin/env python3
"""Fail-closed host regressions for protected master-context Pages recovery."""
from copy import deepcopy
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
print("PASS: protected LAB Pages recovery gate, no firmware rebuild, strict SHA")
