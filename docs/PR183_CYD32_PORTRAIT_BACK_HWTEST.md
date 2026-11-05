# PR 183 — 3.2 ST7798 rotated portrait: missing BACK in Settings/Power Saver

**Reported hardware symptom (November 4, 2026):** After rotating the 3.2-inch CYD while Settings is open, the bottom Back button is missing in the Power Saving menu. Photo also shows the Settings list with stale, rotated content in the bottom reserved Back strip.

## Implementation

- 3.2 CYD portrait: repaint **only** the bottom pinned Back/Up strip on the physical TFT panel after sprite/banded pushes, so viewport clipping and old-orientation remnants cannot hide the navigation control.
- Retain the existing Back hit rectangle tied to real panel `tft.width()/tft.height()`; no change to power settings or flash preferences.
- Settings subpages still show **[ UP ]**; Desk page retains its split **[ OK ] / [ UP ]** strip.
- Invalidate cached row hashes when exiting the direct-overlay mode so a later screen or landscape rotation cannot inherit painted Back-strip pixels.
- Limit corrective repaint to **CYD32 portrait**; other boards and landscape rendering remain unchanged.

## Hardware verification: mandatory

1. Boot **3.2-inch ST7798** without erase-flash. Verify touch, saved settings and normal display.
2. Settings main page in landscape: **[ BACK ]** visible and works.
3. Tap rotate to portrait while Settings open; check **[ BACK ]** remains visible at bottom and returns to CLEAR.
4. In portrait: Settings → System/Appearance/Watch: **[ UP ]** visible and returns to parent; Desk's **[ OK ]**/**[ UP ]** remain independent.
5. Enter Settings → **Power Saver** in portrait. **[ BACK ]** is visible and exits to Settings. Scroll to the last option then repeat.
6. Rotate back to landscape on Settings, revisit Power Saver and confirm no stale bottom pixels or dead touch region.
7. Repeat with/without attached GPS, and after changing WiFi state (which may activate low-memory multi-band recovery), plus portrait rotation 0 and 2.
8. Observe any bottom-strip flicker, new stale remnants, serial panics, and normal scanning/SquachMesh.
9. Confirm 2.8 ST7789 does not regress (build + hardware if possible).

Draft only. Do not merge without explicit hardware approval; do not release/tag production.
