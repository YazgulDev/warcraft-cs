$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
# Standalone checkouts start without build output; each numerical regression runs without game files.
New-Item -ItemType Directory -Path (Join-Path $root 'build') -Force | Out-Null
foreach ($name in @('movement','mouse-look','hitboxes','melee','combat-damage','gameplay-settings','recoil')) {
    & (Join-Path $PSScriptRoot "test-$name.ps1")
}
& (Join-Path $PSScriptRoot 'test-tree-and-wheel.ps1')
Write-Output 'All nine native-independent C++ regression suites passed.'
