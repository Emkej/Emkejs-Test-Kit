# Changelog

All notable changes to Emkejs-Test-Kit will be documented in this file.

## [0.1.0-alpha.2] - 2026-04-06
- Added the full in-game debug toolkit with Health, Stats, Teleport, Inventory, Spawn, and Construction tabs in one panel.
- Added Mod Hub-backed settings for the enabled toggle, hotkey binding, dangerous-action confirmation, debug logging, panel sizing, and per-tab width controls.
- Added saved teleport locations with starter destinations plus inventory weapon and armour quality selection.
- Added custom spawn faction selection with searchable results and target-squad faction inheritance when no custom faction is selected.
- Added a Construction tab that finishes supported player-owned construction targets.
- Fixed repeated spawn placement stacking, custom-faction multi-spawn instability, and selected-target restoration after spawn and inventory actions.
- Removed the Boost.System runtime DLL dependency from the package.
- Split the implementation into focused runtime modules for panel, config, health, inventory, teleport, spawn, and construction responsibilities.

## [0.1.0-alpha.1] - 2026-03-19
- Initial mod scaffold created.
