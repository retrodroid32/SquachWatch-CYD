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


# ---------------------------------------------------------------------------
# TFT_eSPI, ESP32-C5. Only for the nm-cyd-c5 environment.
#
# TFT_eSPI 2.5.43 is upstream's newest release (March 2024) and has no C5
# branch, so it falls through to the plain-Xtensa processor and dies on
# VSPI_HOST and SPI_MOSI_DLEN_REG. This drops RockBase's C5 processor port
# into the copy PlatformIO downloaded and adds the two #elif lines that reach
# it. See boards/nm-cyd-c5-tft_espi/README.md.
#
# It raises rather than warns on anything unexpected. A display driver that
# silently did not get patched is a solid white screen on a board that is
# otherwise running perfectly, which is a miserable thing to debug -- far
# better for the BUILD to stop and say so.
def patch_tft_espi_for_c5(env):
    import shutil

    proj = env.subst("$PROJECT_DIR")
    src = os.path.join(proj, "boards", "nm-cyd-c5-tft_espi")
    lib = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env["PIOENV"], "TFT_eSPI")

    if not os.path.isdir(lib):
        raise Exception(
            "TFT_eSPI is not installed at %s yet, so the ESP32-C5 processor "
            "port cannot be applied. This script must run AFTER the library "
            "manager." % lib
        )

    for name in ("TFT_eSPI_ESP32_C5.c", "TFT_eSPI_ESP32_C5.h"):
        s = os.path.join(src, name)
        d = os.path.join(lib, "Processors", name)
        if not os.path.isfile(s):
            raise Exception("missing %s -- boards/nm-cyd-c5-tft_espi is incomplete" % s)
        # Copy every build: the library can be reinstalled underneath us, and
        # copying 50 KB is cheaper than the failure mode of not copying it.
        shutil.copyfile(s, d)

    # The two dispatch lines. Anchored on the C3 branch that is immediately
    # above where C5 belongs in both files.
    edits = (
        ("TFT_eSPI.h",
         '  #include "Processors/TFT_eSPI_ESP32_C3.h"\n',
         '#elif defined(CONFIG_IDF_TARGET_ESP32C5)\n'
         '  #include "Processors/TFT_eSPI_ESP32_C5.h"\n'),
        ("TFT_eSPI.cpp",
         '    #include "Processors/TFT_eSPI_ESP32_C3.c" // Tested with SPI (8-bit parallel will probably work too!)\n',
         '  #elif defined(CONFIG_IDF_TARGET_ESP32C5)\n'
         '    #include "Processors/TFT_eSPI_ESP32_C5.c"\n'),
    )
    for fname, anchor, addition in edits:
        path = os.path.join(lib, fname)
        with open(path, "r", encoding="utf-8", errors="surrogateescape") as f:
            text = f.read()
        if "TFT_eSPI_ESP32_C5" in text:
            continue                      # already patched; runs are idempotent
        if text.count(anchor) != 1:
            raise Exception(
                "%s: expected exactly one ESP32-C3 include to anchor the C5 "
                "branch to, found %d. TFT_eSPI has changed shape -- check "
                "whether it now supports the C5 on its own."
                % (fname, text.count(anchor))
            )
        text = text.replace(anchor, anchor + addition)
        with open(path, "w", encoding="utf-8", errors="surrogateescape") as f:
            f.write(text)
        print("TFT_eSPI: added ESP32-C5 branch to %s" % fname)


if env["PIOENV"] == "nm-cyd-c5":
    patch_tft_espi_for_c5(env)
