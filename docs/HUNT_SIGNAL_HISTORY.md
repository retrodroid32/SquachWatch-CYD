# HUNT Signal History — classic CYD LAB

A passive **display-only** HUNT mode enhancement for the fork's supported
2.8-inch ST7789 and 3.2-inch ST7798 boards, portrait and landscape.

## How to use
1. Add a BLE or Wi-Fi target to HUNT, then open its gauge.
2. Tap **GRAPH** on the upper-left of the HUNT screen. Tap **GAUGE** to return.
3. The chart shows ~40 seconds of one-per-second RSSI snapshots, newest on
   the right, scaled from **-100 to -30 dBm**. Lines break on radio loss.
4. **PEAK** is the strongest signal observed since this visit to the
   target's HUNT gauge began; it is historical, **not** a live strength.
5. **LAST** counts seconds since the target was last observed. The graph
   status reads **NO SIGNAL** when the existing BLE/Wi-Fi freshness window
   expires, even if older graph points or the peak remain visible.
6. For Wi-Fi hunts, the chart shows the last reported channel; BLE hunts
   show BLE. This does **not** imply direction, location, or accurate range.

## Invariants
- Existing gauge needle, RSSI, trend, CAUGHT and live/no-signal thresholds
  remain controlled by the original coherent `huntSignalSnapshot()`.
- A 40-sample static ring (<100 bytes plus counters) is advanced only on
  the first render pass and at most once per second; no dynamic allocations,
  NVS, SD writes, background worker, or radio reconfiguration.
- Tap region is confined to (8..72,21..45), above the gauge body; BACK and
  STOP retain their original geometry and behavior.
- HUNT targets and radio channel-focus logic remain unchanged.
- New history resets when entering the HUNT gauge for a fresh visit. Exiting
  and returning creates a new history, without removing the target.
- Peak and chart are exploratory telemetry; RSSI depends on obstacles,
  orientation and advertising intervals and cannot give exact distance.

## Hardware validation gate
2.8 ST7789, then 3.2 ST7798; each portrait and landscape:
1. Open a BLE HUNT from LOG or RAWSCAN. Tap GRAPH, check smooth graph and
   latest point/PEAK; tap GAUGE, verify same live needle.
2. Move source closer/farther; PEAK must retain strongest seen value and
   never replace the live numeric reading. Stop transmission: **NO SIGNAL**
   and a graph gap must appear after the existing freshness timeout, with
   LAST increasing. Resume source, graph should reconnect at the right.
3. Repeat with Wi-Fi source; channel label displays observed channel, and
   the normal adaptive scan resumes after source loss.
4. Confirm BACK goes to HUNT roster, STOP removes the selected target,
   keyboard/screens/notifications do not redraw incorrectly, and the
   2.8/3.2 render correctly at Tiny and Huge button-font settings.
5. Run SD pull/reinsert, GPS UBX at 115200, Wi-Fi USE, WATCH and
   SquachMesh smoke checks. Report PASS/FAIL per board.

No merging, tagging or production release until physical acceptance.
