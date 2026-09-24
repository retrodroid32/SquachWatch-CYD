# Stamps the build identity into firmware as FIRMWARE_VERSION.
#
# Official tags keep the clean release number ("1.20.1"). Untagged source
# builds are deliberately distinguishable ("1.20.1-dev+deadbeef"), and lab
# tags keep their lab-* identity. Release/lab CI can set SQW_VERSION explicitly.
Import("env")
import os
import subprocess


PROJECT = env["PROJECT_DIR"]


def git(*args):
    return subprocess.check_output(
        ["git", *args],
        cwd=PROJECT,
        stderr=subprocess.DEVNULL,
    ).decode().strip()


def intended_version():
    try:
        with open(os.path.join(PROJECT, "VERSION"), encoding="utf-8") as f:
            return f.read().strip()
    except Exception:
        return ""


def get_version():
    # CI/bench override. release-flasher.yml sets this from its already
    # validated release/lab identity, so the firmware and manifest agree.
    forced = os.environ.get("SQW_VERSION", "").strip()
    if forced:
        return forced

    intended = intended_version()
    try:
        tags = [x.strip() for x in git("tag", "--points-at", "HEAD").splitlines() if x.strip()]
        if intended and ("v" + intended) in tags:
            return intended
        labs = sorted(t for t in tags if t.startswith("lab-"))
        if labs:
            return labs[0]

        short = git("rev-parse", "--short=8", "HEAD") or "unknown"
        if intended:
            dirty = bool(git("status", "--porcelain", "--untracked-files=no"))
            return "%s-dev+%s%s" % (intended, short, ".dirty" if dirty else "")
        desc = git("describe", "--tags", "--always", "--dirty")
        return desc if desc else "unknown"
    except Exception:
        return (intended + "-dev+unknown") if intended else "unknown"


env.Append(BUILD_FLAGS=['-DFIRMWARE_VERSION=\\"%s\\"' % get_version()])

# The environment name, as SQW_ENV. A Bluetooth update is signed for exactly
# one build and the board checks the signature against its own name.
env.Append(BUILD_FLAGS=['-DSQW_ENV=\\"%s\\"' % env["PIOENV"]])
