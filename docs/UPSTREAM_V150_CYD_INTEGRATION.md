# Upstream v1.50 integration — phased CYD validation plan

Upstream baseline: skizzophrenic/SquachWatch-CYD v1.50.0 (2026-11-22).
Fork integration stays on LAB branches until owner hardware verification and explicit merge approval.

## Phase A: Detection signatures (this PR)
- Add vendor-registered Wyze F0:C8:8B and 80:48:2C camera prefixes.
- Add SimpliSafe F8:51:28 with MED confidence: company also makes non-camera products.
- Correct legacy HC-05 module prefix 98:D3:00 to the manufacturer-registered 98:D3:31; LOW confidence, not proof of skimming.
- Source-level regression and full host/emulator/board build gates. No UI, animation, GPS, SD, Wi-Fi or mesh changes.
- Physical test: compare known test broadcasts, ensure no false ALPR or camera label; CYD 2.8 and 3.2 scan and Watch/Hunt smoke.

## Phase B: Reliability and alert correctness
- Complete v1.44/v1.45 memory and security audit in #239; do not independently transplant incompatible alert queues.
- Compare v1.50 AirTag owner-state matching, AirPods false-positive fix, and queued alert cards against fork's personalized Watch/Hunt/SquachMesh flow.
- Create dedicated evidence-backed LAB PR with receiver/emitter tests and free/contiguous heap telemetry.

## Phase C: Trail and wardriving
- Compare include/trail.h, src/places.cpp, alert UI, existing log timestamps; privacy mode and multi-day Watch/Hunt semantics must be preserved.
- Integrate WARDRIVE page and browser WiGLE upload separately from on-board S3-only upload.
- Treat network-derived location as approximate; validate privacy and no-GPS behavior.

## Phase D: Visual optional features
- Alert card skins and DU5K theme in dedicated LAB change.
- Random encounters, field journal and Goblin visitor in separate LAB change.
- Explicitly preserve user-verified CYD 3.2 ST7798 rendering/animation and avoid incremental heap costs on CYD 2.8.

## Dependencies and release conditions
- #255: CYD32 SD-in boot fallback based on R11; firmware Build #1059 passed, physical SD-in boot still required.
- #239: upstream earlier security PR still draft. Refresh and verify before any merge.
- New LAB PRs must build both ST7789/ST7798 and pass OTA partition/headroom guards.
- No mass upstream master merge; no production tag, automatic PR merge, or branch deletion.
