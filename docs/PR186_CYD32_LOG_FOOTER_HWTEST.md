# PR186 — 3.2 ST7798 LOG footer redraw

**Physical report, Nov 5, 2026:** After the user confirmed the 3.2 GUI/scan improvements worked in combined PR182–185 LAB firmware, the bottom of the LOG screen still visibly redrew. Do not merge until validated on the board.

## Technical audit
- `uiLogTick` covers the animated list background from `y=0` through `bar.y-4`.
- Its three exit paths (empty, EVENTS, DEVICES) previously painted only the button rectangles; the unused footer gaps between/under buttons inherited pixels from previous screens or the other recovery band.
- Drawing text on LOG may leave a non-default font on its reused sprite; `Theme::drawButton` inherits font and may change button metrics between passes.

## Scoped fix
- Common `drawLogFooter` uses the same `Theme::computeButtonBar` geometry but first clears `[bar.y-4, screen bottom)` to `Theme::BG` in the **offscreen sprite**.
- `DrawBand::has()` avoids painting the footer in unrelated recovery bands.
- Set font #1, text size 1, no wrap before `Theme::drawButtonBar`; all three paths use the same helper. The row hash pusher still skips unchanged footer rows.
- No changes to LOG data, scroll, touch, scan, Wi-Fi, mesh, or 2.8 rendering configuration.

## Physical verification (both 3.2 and 2.8 CYDs)
1. On 3.2, open LOG → DEVICES with data; check bottom SCAN/LOG/CLR button strip for flashes/partial background/stale pixels, and tap each button.
2. Switch to EVENTS and an empty LOG, with any animated background and again with BLACK; bottom strip stays opaque and stationary, not flickering during normal updates.
3. Scroll rapidly, open an entry's WATCH/HUNT/INFO overlay then dismiss; buttons reappear intact, no residual fragments or doubled text.
4. Rotate to portrait and landscape while LOG open; verify correct footer size/position, touch hit boxes, and transition to Settings.
5. Exercise BLE→Wi-Fi scan while scanning; check previously fixed scan buttons.
6. After Wi-Fi USE and SD/GPS memory pressure, revisit LOG to exercise the multi-band fallback; record `[frame] ... active renderer bands=N`.
7. Confirm 2.8 ST7789 LOG and other screens unchanged. Do not erase NVS.

**Unmerged draft, LAB only**. No production release/tag.
