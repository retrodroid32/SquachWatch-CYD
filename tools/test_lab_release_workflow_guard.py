#!/usr/bin/env python3
"""Guard against a LAB-branch GitHub Pages job breaking firmware release."""
from pathlib import Path

root = Path(__file__).resolve().parents[1]
wf = (root / ".github/workflows/release-flasher.yml").read_text(encoding="utf-8")
req = (root / ".github/lab-build-request.txt").read_text(encoding="utf-8")
assert "lab/hunt-history-peak-graph-r1-20261111" in wf
assert "paths:\n      - .github/lab-build-request.txt" in wf
assert "Wait for matching PR Build success before LAB publication" in wf
assert "TARGET_SHA: ${{ github.sha }}" in wf
assert "deploy-lab-pages:" not in wf
assert "environment:\n      name: github-pages" not in wf
assert 'if: steps.ver.outputs.is_lab != \'true\'' in wf
assert "name: pages-site" in wf
assert req.splitlines()[0] == "HUNT-HISTORY-THREE-FOOTER-BUTTONS-R13"
print("PASS: LAB release uses Build gate and publishes artifact without a blocked Pages job")
