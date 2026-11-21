#!/usr/bin/env python3
"""Pages deployment recovery regression: exact artifact-id and secure OIDC.

This checks the workflow wiring and JavaScript syntax without requiring
Pages write permissions during PR CI. A deployed site still requires a
successful GitHub Pages workflow run.
"""
from pathlib import Path
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[1] /
          ".github/workflows/publish-pages.yml").read_text()
assert "VERIFIED_LAB_SOURCE_RUN_ID" in source
assert "gh run download" in source and "pages-site" in source
assert "github-pages-attempt-${{ github.run_attempt }}" in source
assert "id: pages_artifact" in source
assert "jq --arg name" in source
assert "artifact_id=$artifact_id" in source
assert "PAGES_ARTIFACT_ID: ${{ steps.pages_artifact.outputs.artifact_id }}" in source
assert "actions/github-script@v8" in source
assert "core.getIDToken()" in source
assert "core.setSecret(oidcToken)" in source
assert "'POST /repos/{owner}/{repo}/pages/deployments'" in source
assert "'GET /repos/{owner}/{repo}/pages/deployments/{deploymentId}'" in source
assert "artifact_id: artifactId" in source
assert "pages_build_version: context.sha" in source
assert "oidc_token: oidcToken" in source
assert "core.setOutput('page_url', deployment.page_url)" in source
assert "actions/deploy-pages@" not in source

anchor = "      - name: Deploy verified artifact to GitHub Pages\n"
body = source.split(anchor, 1)[1]
marker = "          script: |\n"
assert marker in body
script = body.split(marker, 1)[1]
assert script.endswith("\n")
js = "\n".join(line[12:] for line in script.splitlines()) + "\n"
with tempfile.TemporaryDirectory() as tmp:
    src = Path(tmp) / "deployment.cjs"
    src.write_text("async function main() {\n" + js + "}\n")
    subprocess.run(["node", "--check", str(src)], check=True)

print("PASS: one verified artifact, REST Pages deployment, masked OIDC, JS syntax")
