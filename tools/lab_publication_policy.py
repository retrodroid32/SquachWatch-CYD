#!/usr/bin/env python3
"""Trusted default-branch policy for the shared LAB installer."""
import re

CANONICAL_LAB_BRANCH = "lab/v151-r161-scanner-20261127"

def allowed(run, canonical_head, repo, recovery=False):
    if (run.get("head_repository") or {}).get("full_name") != repo:
        return False
    branch = run.get("head_branch", "")
    if branch == CANONICAL_LAB_BRANCH:
        return bool(re.fullmatch(r"[0-9a-f]{40}", canonical_head or "")) and run.get("head_sha") == canonical_head
    # A real production version tag is handled by the existing release pipeline;
    # no experimental lab tag, older branch or dispatch can publish the shared LAB.
    return (not recovery and run.get("event") == "push"
            and bool(re.fullmatch(r"v[0-9]+[.][0-9]+[.][0-9]+", branch)))

if __name__ == "__main__":
    import argparse, json
    p = argparse.ArgumentParser()
    p.add_argument("--run", required=True)
    p.add_argument("--canonical-head", required=True)
    p.add_argument("--repo", required=True)
    p.add_argument("--recovery", action="store_true")
    a=p.parse_args()
    with open(a.run, encoding="utf-8") as f:
        run=json.load(f)
    if not allowed(run,a.canonical_head,a.repo,a.recovery):
        raise SystemExit("Refusing stale or non-authoritative LAB publication")
    print("PASS: authoritative Pages source")
