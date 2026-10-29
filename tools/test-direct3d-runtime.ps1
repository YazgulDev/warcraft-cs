param([string]$Directory='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/prebuilt-runtime.ps1')
. (Join-Path $root 'setup/direct3d-runtime.ps1')
if(!$Directory){$Directory=Join-Path $root '.local/dependencies/direct3d'}
Assert-Direct3DFiles $Directory
$trial=Join-Path $root ('.local/direct3d-install-tests-'+[guid]::NewGuid().ToString('N'))
$runtime=Join-Path $trial 'Game'
New-Item -ItemType Directory -Path $runtime -Force | Out-Null
function Reject([scriptblock]$Action) {
    $failed=$false;try{& $Action | Out-Null}catch{$failed=$true}
    if(!$failed){throw 'Unsafe Direct3D installation unexpectedly succeeded.'}
}
# Real redistributable bytes exercise installation, replacement backups and all checksum gates offline.
Reject {Install-Direct3DRuntime $Directory $runtime}
Set-Content -LiteralPath (Join-Path $runtime '.warcraft-cs-private-runtime') -Value 'owned synthetic runtime'
Set-Content -LiteralPath (Join-Path $runtime 'd3d8.dll') -Value 'previous private wrapper'
New-Item -ItemType Directory -Path (Join-Path $runtime 'Save') -Force | Out-Null
Set-Content -LiteralPath (Join-Path $runtime 'Save/progress.w3z') -Value 'untouched save'
Install-Direct3DRuntime $Directory $runtime
Assert-Direct3DFiles $runtime
$backups=@(Get-ChildItem -LiteralPath (Join-Path $runtime 'WarcraftCS/backups/graphics') -Recurse -File)
if($backups.Count -ne 1 -or (Get-Content -LiteralPath $backups[0].FullName -Raw).Trim() -ne 'previous private wrapper'){throw 'Previous graphics wrapper not preserved.'}
Install-Direct3DRuntime $Directory $runtime
if(@(Get-ChildItem -LiteralPath (Join-Path $runtime 'WarcraftCS/backups/graphics') -Recurse -File).Count -ne 1){throw 'Unchanged graphics files backed up again.'}
if((Get-Content -LiteralPath (Join-Path $runtime 'Save/progress.w3z') -Raw).Trim() -ne 'untouched save'){throw 'Graphics setup changed progress.'}
foreach($name in @('d3d8to9-BSD-2-Clause.txt','Microsoft-DXSDK-D3DX.txt')) {
    if(!(Test-Path -LiteralPath (Join-Path $runtime ('WarcraftCS/notices/'+$name)))){throw 'Redistributable license missing.'}
}
$corrupt=Join-Path $trial 'corrupt';New-Item -ItemType Directory -Path $corrupt -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $Directory 'd3d8.dll') -Destination $corrupt
Copy-Item -LiteralPath (Join-Path $Directory 'direct3d-runtime.json') -Destination $corrupt
Reject {Install-Direct3DRuntime $corrupt $runtime}
Copy-Item -LiteralPath (Join-Path $Directory 'd3dcompiler_43.dll') -Destination $corrupt
Set-Content -LiteralPath (Join-Path $corrupt 'd3dx9_43.dll') -Value 'corrupt'
Reject {Install-Direct3DRuntime $corrupt $runtime}
Copy-Item -LiteralPath (Join-Path $Directory 'd3dx9_43.dll') -Destination $corrupt -Force
Set-Content -LiteralPath (Join-Path $corrupt 'd3d8.dll') -Value 'corrupt bridge'
Reject {Install-Direct3DRuntime $corrupt $runtime}
Assert-Direct3DFiles $runtime
Write-Output 'PASS Direct3D app-local pinned files, missing/corrupt rejection, repeat setup, original-wrapper backup, notices and save preservation.'
