$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
# Standalone checkouts start without build output; each numerical regression runs without game files.
New-Item -ItemType Directory -Path (Join-Path $root 'build') -Force | Out-Null
# Camera edge ownership and static mesh contacts join the synthetic adapter suites;
# none of these checks controls an active Warcraft game or campaign.
foreach ($name in @('window-input','fog-of-war','diagnostic-log','native-frame-rate','native-frame-presentation','native-camera-far-clip','map-camera-guard','movement','movement-obstacles','static-collision','warcraft-collision','fullscreen-view','minimap-layout','native-minimap','fps-projection','world-labels','mouse-look','hitboxes','melee','combat-damage','gameplay-settings','recoil')) {
    & (Join-Path $PSScriptRoot "test-$name.ps1")
}
& (Join-Path $PSScriptRoot 'test-tree-and-wheel.ps1')
& (Join-Path $PSScriptRoot 'test-third-person.ps1')
& (Join-Path $PSScriptRoot 'test-native-hover-view.ps1')
# Attack scope/lifecycle regressions use synthetic native entry points, with no game files.
& (Join-Path $PSScriptRoot 'test-native-weapon-attack.ps1')
& (Join-Path $PSScriptRoot 'test-weapon-target-filter.ps1')
# Health bars verify native filtering and actual WGL depth/state without taking input focus.
& (Join-Path $PSScriptRoot 'test-health-bars.ps1')
# Configurable spell slots and explicit native order ownership need no interactive game input.
& (Join-Path $PSScriptRoot 'test-spells.ps1')
& (Join-Path $PSScriptRoot 'test-spell-hud.ps1')
# HUD text is checked against actual WGL pixels under inherited upload/render state.
& (Join-Path $PSScriptRoot 'test-hud-font.ps1')
& (Join-Path $PSScriptRoot 'test-unit-hud.ps1')
& (Join-Path $PSScriptRoot 'test-interactive-inventory.ps1')
& (Join-Path $PSScriptRoot 'test-squad-reticle.ps1')
& (Join-Path $PSScriptRoot 'test-army-ai.ps1')
& (Join-Path $PSScriptRoot 'test-shotgun-reload.ps1')
& (Join-Path $PSScriptRoot 'test-glock-fire.ps1')
& (Join-Path $PSScriptRoot 'test-viewmodel.ps1')
& (Join-Path $PSScriptRoot 'test-grenades.ps1')
# Optional RPG flight, private-asset discovery and actual GL effects belong to the complete release checks.
& (Join-Path $PSScriptRoot 'test-rpg.ps1')
& (Join-Path $PSScriptRoot 'test-halflife.ps1')
# Buying/sky decoding are independent of Warcraft and use only synthetic/private-free test inputs.
& (Join-Path $PSScriptRoot 'test-buy-menu.ps1')
& (Join-Path $PSScriptRoot 'test-dollar-loot.ps1')
& (Join-Path $PSScriptRoot 'test-weapon-selection.ps1')
# Alternate graphics and host selection use synthetic fixtures and a real hidden D3D device, without Warcraft files.
& (Join-Path $PSScriptRoot 'test-d3d9.ps1')
& (Join-Path $PSScriptRoot 'test-host-builds.ps1')
& (Join-Path $PSScriptRoot 'test-miles-forwarders.ps1')
Write-Output 'All native-independent regression suites passed.'
