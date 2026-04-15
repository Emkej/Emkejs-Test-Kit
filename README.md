## Emkejs Test Kit (RE_Kenshi plugin)

This repository is the starter scaffolding for the Emkejs-Test-Kit native RE_Kenshi plugin mod.

## Setup
1. Review .env and adjust local paths as needed (.env.example is kept as a reference copy).
2. Open a PowerShell terminal in this repo.
3. Source the environment script:
   - . .\scripts\setup_env.ps1

This sets:
- KENSHILIB_DEPS_DIR
- KENSHILIB_DIR
- BOOST_INCLUDE_PATH

## Build
You can build in Visual Studio, or via the scripted wrapper:

- .\scripts\build-deploy.ps1

Optional parameters:
- -KenshiPath "H:\SteamLibrary\steamapps\common\Kenshi"
- -Configuration "Release"
- -Platform "x64"

## Mod Hub SDK
This repo includes the optional Mod Hub SDK checkout in tools/mod-hub-sdk.
The generated checkout keeps consumer-facing SDK files only; reference docs stay in the template repo/upstream SDK repo.

Generate the standard Mod Hub adapter scaffold with:

- ./scripts/init-mod-template.sh --with-hub
- ./scripts/init-mod-template.ps1 -WithHub

That scaffold creates src/mod_hub_consumer_adapter.h and src/mod_hub_consumer_adapter.cpp.

Sync and validate it with:

- ./scripts/sync-mod-hub-sdk.sh

Use --skip-pull for validation-only mode.
## Deploy layout
Mod data folder name: Emkejs-Test-Kit

After deploy, expected files:
- [Kenshi install dir]\mods\Emkejs-Test-Kit\Emkejs-Test-Kit.mod
- [Kenshi install dir]\mods\Emkejs-Test-Kit\RE_Kenshi.json
- [Kenshi install dir]\mods\Emkejs-Test-Kit\Emkejs-Test-Kit.dll
- [Kenshi install dir]\mods\Emkejs-Test-Kit\mod-config.json

## Config
`mod-config.json` now drives the step 1 panel shell:
- `enabled`
- `toggle_panel_key`
- `toggle_panel_ctrl`
- `toggle_panel_shift`
- `toggle_panel_alt`
- `start_hidden`
- `start_collapsed`
- `logging_level`
- `confirm_dangerous_actions`
- `panel_width`
- `panel_min_expanded_height`
- `panel_max_expanded_height`
- `panel_header_title_font_height`
- `panel_collapse_button_size`
- `panel_close_button_size`
- `panel_body_overlap`

The in-game panel shell currently provides show/hide, collapse/expand, placeholder target summary text, shell action buttons, and an in-panel status line. Target inspection and real state forcing come in later plan steps.
