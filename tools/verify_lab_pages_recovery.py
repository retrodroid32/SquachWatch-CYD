#!/usr/bin/env python3
"""Validate a manual LAB Pages recovery without rebuilding untrusted firmware.

Called only from the trusted default-branch publish-pages.yml. The source
release run may be red, but *only* because github-pages rejected its deploy
job before runner allocation. CI, tests, firmware and site packaging must
all have succeeded at the exact requested SHA.
"""
import argparse
import json
import re


RELEASE_WORKFLOW = "Build and release firmware"
DEPLOY_JOB = "Publish passed LAB firmware to web flasher"
REQUIRED = ("preflight", "ci-gate", "build-and-release")


def verify(run: dict, payload: dict, sha: str, repo: str) -> None:
    if not re.fullmatch(r"[0-9a-f]{40}", sha):
        raise ValueError("A complete 40-character hexadecimal SHA is required")
    if run.get("name") != RELEASE_WORKFLOW or run.get("event") != "workflow_dispatch":
        raise ValueError("Source must be a dispatched Build and release firmware workflow")
    if run.get("status") != "completed" or run.get("conclusion") != "failure":
        raise ValueError("Source must be a completed LAB release with Pages-only failure")
    if run.get("head_sha") != sha:
        raise ValueError("Exact commit SHA mismatch; refusing to publish another build")
    if not str(run.get("head_branch", "")).startswith("lab/"):
        raise ValueError("Only explicitly named lab/* branches may use recovery")
    if (run.get("head_repository") or {}).get("full_name") != repo:
        raise ValueError("Source is not a build of this repository")
    jobs = payload.get("jobs", [])
    if payload.get("total_count", len(jobs)) != len(jobs):
        raise ValueError("Incomplete jobs page; refusing to assume all gates passed")
    for required in REQUIRED:
        matching = [j for j in jobs if j.get("name") == required]
        if len(matching) != 1 or matching[0].get("conclusion") != "success":
            raise ValueError(f"Required source gate {required} did not pass")
    failed = [j.get("name") for j in jobs if j.get("conclusion") != "success"]
    if failed != [DEPLOY_JOB]:
        raise ValueError(f"Only the isolated Pages deploy job may fail: {failed}")
    print(f"PASS: verified LAB artifact recovery for {repo} at {sha}")


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument("--run", required=True)
    p.add_argument("--jobs", required=True)
    p.add_argument("--sha", required=True)
    p.add_argument("--repo", required=True)
    args = p.parse_args()
    with open(args.run, encoding="utf-8") as f:
        run = json.load(f)
    with open(args.jobs, encoding="utf-8") as f:
        jobs = json.load(f)
    verify(run, jobs, args.sha, args.repo)


if __name__ == "__main__":
    main()
