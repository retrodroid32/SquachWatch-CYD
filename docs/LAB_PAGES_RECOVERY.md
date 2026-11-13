# Recover a validated LAB Pages artifact without rebuilding firmware

This is a **manual emergency recovery path** for a LAB release whose
preflight, CI gate, firmware build, binary OTA-size checks, site verification,
and `pages-site` upload succeeded, but whose **final GitHub Pages
environment job** was rejected before any runner started.

The recovery job runs only from the repository's protected **master** context.
It **does not** weaken the `github-pages` deployment branch restriction,
merge feature code, create tags/releases, or rebuild firmware.

## Recovery after this change is explicitly approved and merged

This PR includes the narrowly scoped
`.github/lab-pages-recover-request.json` file containing the exact
failed run ID and SHA. **Merging it into master triggers the recovery
automatically** through the `push` event, *only when that request file
changes*. GitHub Pages sees a master-context deployment, not a LAB-branch
deployment. The recovery downloads the existing verified site artifact and
never recompiles firmware. No further manual dispatch is required.

For future failures, the same publisher retains a manual recovery mode:
open **Actions > Publish web flasher > Run workflow**, select `master`,
provide a completed failed release run ID and its exact full SHA, then run.
Never trigger a recovery until its original Build and release firmware
artifact has passed all non-Pages jobs.

The run's artifact retention is only three days; recovery must happen
before GitHub expires the archive. If expired, start a new LAB release
**only after** its corresponding PR Build is green.

## Safety gates

The validation script rejects a source run unless:

1. It is the repository's own completed `Build and release firmware`
   `workflow_dispatch` event from a `lab/*` branch.
2. Its head SHA matches the fully specified expected SHA.
3. Its `preflight`, `ci-gate`, and `build-and-release` jobs all succeeded.
4. Its **only** non-successful job is the known Pages deploy job.
5. Its site artifact exists and contains required board/version/update/emulator
   files before attempting Pages upload or deploy.

The existing automatic `workflow_run` production publishing path remains
unchanged. GitHub Pages environment protection continues to gate deployment.
