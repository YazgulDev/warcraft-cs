$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
# Standalone checkouts start without build output; each numerical regression runs without game files.
New-Item -ItemType Directory -Path (Join-Path $root 'build') -Force | Out-Null
# Fog restoration is a platform-adapter contract with synthetic native switches.
foreach ($name in @('window-input','fog-of-war','diagnostic-log','native-frame-rate','native-frame-presentation','native-camera-far-clip','movement','movement-obstacles','warcraft-collision','fullscreen-view','minimap-layout','native-minimap','fps-projection','world-labels','mouse-look','hitboxes','melee','combat-damage','gameplay-settings','recoil')) {
    & (Join-Path $PSScriptRoot "test-$name.ps1")
}
& (Join-Path $PSScriptRoot 'test-tree-and-wheel.ps1')
& (Join-Path $PSScriptRoot 'test-third-person.ps1')
# Attack scope/lifecycle regressions use synthetic native entry points, with no game files.
& (Join-Path $PSScriptRoot 'test-native-weapon-attack.ps1')
& (Join-Path $PSScriptRoot 'test-weapon-target-filter.ps1')
# Health bars verify native filtering and actual WGL depth/state without taking input focus.
& (Join-Path $PSScriptRoot 'test-health-bars.ps1')
# Configurable spell slots and explicit native order ownership need no interactive game input.
& (Join-Path $PSScriptRoot 'test-spells.ps1')
& (Join-Path $PSScriptRoot 'test-spell-hud.ps1')
# Buying/sky decoding are independent of Warcraft and use only synthetic/private-free test inputs.
& (Join-Path $PSScriptRoot 'test-buy-menu.ps1')
Write-Output 'All native-independent regression suites passed.'
