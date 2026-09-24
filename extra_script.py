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
    # same tree. Never set in a release build.
    forced = os.environ.get("SQW_VERSION", "").strip()
    if forced:
        return forced
    version_file = os.path.join(env["PROJECT_DIR"], "VERSION")
    try:
        with open(version_file, encoding="utf-8") as f:
            v = f.read().strip()
        if v:
            return v
    except Exception:
        pass
    try:
        v = subprocess.check_output(
            ["git", "describe", "--tags", "--always", "--dirty"],
            stderr=subprocess.DEVNULL,
        ).decode().strip()
        return v if v else "unknown"
    except Exception:
        return "unknown"


env.Append(BUILD_FLAGS=['-DFIRMWARE_VERSION=\\"%s\\"' % get_version()])

# The environment name, as SQW_ENV. A Bluetooth update is signed for exactly one
# build and the board checks the signature against its own name, so an image
# for a different board -- a different display driver, say -- is refused
# rather than installed as a white screen. See include/ota_ble.h.
env.Append(BUILD_FLAGS=['-DSQW_ENV=\\"%s\\"' % env["PIOENV"]])
