# Inventory Weapon Quality Implementation Plan

## Summary
- Add a `Quality` selector to the Inventory tab for spawned weapons and crossbows.
- Keep armor out of scope for this slice.
- Keep the large inventory and panel files to thin integration only by placing quality logic in `src/test_kit_inventory_quality.h/.cpp`.

## Key Changes
- Add `src/test_kit_inventory_quality.h/.cpp` for:
  - quality option discovery from the selected item
  - session-only remembered quality label
  - quality dropdown state and selection resolution
- Add Inventory tab UI controls:
  - `Quality` label at `BuildBodyCoord(20, 502, kPanelWidth - 40, 18)`
  - `Quality` dropdown at `BuildBodyCoord(20, 524, kPanelWidth - 40, 30)`
  - shrink the item results list to `BuildBodyCoord(20, 410, kPanelWidth - 40, 86)`
- Thread the resolved selection through the weapon spawn path so weapons/crossbows use the chosen `manufacturer/model/level`.
- Keep food, general items, and armor on their existing paths.

## Test Plan
1. Run `bash ./tools/build-scripts/build.sh`.
2. Verify non-weapon selections show disabled `Default`.
3. Verify weapons with multiple qualities show named options and preselect the implicit default tuple.
4. Verify last-used matching quality is remembered for the current runtime session only.
5. Verify weapon/crossbow spawn status and action-result logs include quality information.

## Assumptions
- Session-only memory resets on save/load transition because `ResetInventoryRuntimeState()` runs there.
- Crossbows may fall back to `Default` when no quality metadata is available.
- No config or Mod Hub changes are required.
