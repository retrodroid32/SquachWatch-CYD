#!/usr/bin/env python3
from lab_publication_policy import allowed, CANONICAL_LAB_BRANCH
REPO="owner/repo"
HEAD="a"*40
old="b"*40
def run(branch=CANONICAL_LAB_BRANCH,sha=HEAD,event="push",repo=REPO):
    return {"head_branch":branch,"head_sha":sha,"event":event,"head_repository":{"full_name":repo}}
assert allowed(run(),HEAD,REPO)
assert allowed(run(),HEAD,REPO,True)
assert not allowed(run(sha=old),HEAD,REPO)
assert not allowed(run(sha=old),HEAD,REPO,True)
assert not allowed(run("lab/tdeck-plus-hardware-firmware-r1-20261124"),HEAD,REPO)
assert not allowed(run("lab/tdeck-plus-hardware-firmware-r1-20261124"),HEAD,REPO,True)
assert not allowed(run("lab-old"),HEAD,REPO)
assert not allowed(run("master"),HEAD,REPO)
assert not allowed(run(repo="other/repo"),HEAD,REPO)
assert not allowed(run(),"",REPO)
assert allowed(run("v1.51.0"),HEAD,REPO)
assert not allowed(run("v1.51.0",event="workflow_dispatch"),HEAD,REPO)
assert not allowed(run("v1.51.0"),HEAD,REPO,True)
print("PASS: canonical LAB publication policy (13 cases)")
