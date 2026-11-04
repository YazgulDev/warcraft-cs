$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'paths.ps1')
# Original map generation checks run without Warcraft, a retail map, or third-party MPQ libraries.
& (Get-WarcraftCsPython $root) (Join-Path $root 'tests/SurfPracticeMapTests.py')
if($LASTEXITCODE){throw 'Surf practice map verification failed.'}
