# Stamps the current git tag/commit into the firmware as FIRMWARE_VERSION,
# shown on the Diary screen (see src/ui_diary.cpp). Falls back to "unknown"
# if git isn't available or this isn't a git checkout at all -- never
# breaks the build over a missing version string.
Import("env")
import os
import subprocess


def get_version():
    # A bench override: SQW_VERSION=9.9.9 makes a build claim a version, so a
    # squad update nudge from it counts as newer on a board built from the
    # same tree. The release workflow also sets it deliberately after it has
    # verified that the vX.Y.Z tag matches VERSION.
    forced = os.environ.get("SQW_VERSION", "").strip()
    if forced:
        return forced

    version_file = os.path.join(env["PROJECT_DIR"], "VERSION")
    base = "unknown"
    try:
        with open(version_file, encoding="utf-8") as f:
            base = f.read().strip() or "unknown"
    except Exception:
        pass

    # Only an exact release tag is allowed to claim the bare VERSION. A normal
    # branch build gets an explicit prerelease identity instead, so a board
    # flashed from master never masquerades as the later vX.Y.Z release.
    try:
        exact = subprocess.check_output(
            ["git", "describe", "--tags", "--exact-match", "HEAD"],
            stderr=subprocess.DEVNULL,
        ).decode().strip()
        if exact in (base, "v" + base):
            return base
    except Exception:
        pass

    try:
        short = subprocess.check_output(
            ["git", "rev-parse", "--short=8", "HEAD"],
            stderr=subprocess.DEVNULL,
        ).decode().strip()
        dirty = subprocess.run(
            ["git", "diff-index", "--quiet", "HEAD", "--"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        ).returncode != 0
        suffix = "-dev." + (short or "unknown")
        if dirty:
            suffix += ".dirty"
        return base + suffix if base != "unknown" else "unknown" + suffix
    except Exception:
        return base + "-dev" if base != "unknown" else "unknown"

env.Append(BUILD_FLAGS=['-DFIRMWARE_VERSION=\\"%s\\"' % get_version()])

# The environment name, as SQW_ENV. A Bluetooth update is signed for exactly one
# build and the board checks the signature against its own name, so an image
# for a different board -- a different display driver, say -- is refused
# rather than installed as a white screen. See include/ota_ble.h.
env.Append(BUILD_FLAGS=['-DSQW_ENV=\\"%s\\"' % env["PIOENV"]])
