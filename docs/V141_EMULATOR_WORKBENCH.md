# v1.41 classic CYD emulator workbench (LAB R2)

This is the **browser presentation** portion of upstream 1.41, transplanted onto
the fork's latest master **3192c7688d09723ec60c897492360ca719ea2423**, after the owner-approved Fonts
PRs #219–#222. It is not a new WebAssembly firmware build.

- Classic **CYD 2.8**: embedded 86.8 mm PCB SVG, screen/cable/controls,
  responsive cutting-mat workspace, zoom and rotation modes, pointer input
  and the existing detection/SquachMesh simulator console.
- **CYD 3.2 ST7798**: **not emulated yet**. Do not reuse 2.8 artwork/driver
  and imply a faithful 3.2. A separate simulator/profile is required.
- T-Deck, T-Watch, Cardputer, C5 and other boards are also intentionally
  unavailable until board-specific compatible WASM binaries exist.
- **BOOT / Pocket Mode** control is capability-gated by exported WASM
  functions; a control being drawn is not evidence of latest firmware support.
- The browser preview artifact uses the **previous checked-in**
  `squachsim.js` and `squachsim.wasm`. New Fonts settings, privacy fixes
  and other newly merged changes are **not** claimed to be present inside
  that old WASM binary.
- The fork's main firmware source, partition table, production LAB manifests
  and real CYD flasher pages are unchanged.

## Review
Download the *lab-pr-N-v141-browser-workbench* artifact from the PR's
successful GitHub Build run. Extract it and, from its parent directory,
run `python3 -m http.server 8000 --directory browser-preview`, then open
`http://localhost:8000/emulator/`.

Check that the PCB screen aligns with the displayed emulator pixels at
different zoom and rotation angles, mouse/touch coordinates work, demos and
virtual mesh are functional, and unsupported features do not appear.
Static and Node syntax checks run in CI. This preview does not transmit
real signals or update the production Pages site.

## Next 1.41 parity steps
1. Rebuild classic CYD WASM from current master; validate exported API and
   font UI to make browser firmware parity a real claim.
2. Build a correctly sized separate 3.2 ST7798 emulator/profile.
3. Recheck 3.2 panel orientation and touch smoke tests, privacy, scan,
   GPS UBX, Wi-Fi USE, SD, WATCH/HUNT, SquachMesh before any firmware merge.
