param([Parameter(Mandatory=$true)][string]$SourceDirectory,[Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$vs=& 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Developer Direct3D preparation requires the same Visual C++ tools as the mod build.'}
$environment=Join-Path $vs 'VC/Auxiliary/Build/vcvars32.bat'
$build=Join-Path $root 'build/direct3d-bridge';New-Item -ItemType Directory -Path $build -Force | Out-Null
$source=[IO.Path]::GetFullPath($SourceDirectory);$output=[IO.Path]::GetFullPath($OutputDirectory)
$names=@('d3d8to9.cpp','d3d8to9_base.cpp','d3d8to9_device.cpp','d3d8to9_index_buffer.cpp','d3d8to9_surface.cpp',
    'd3d8to9_swap_chain.cpp','d3d8to9_texture.cpp','d3d8to9_vertex_buffer.cpp','d3d8to9_volume.cpp','d3d8types.cpp','interface_query.cpp')
$sources=@($names | ForEach-Object {'"'+(Join-Path $source ('source/'+$_))+'"'})
$dll=Join-Path $output 'd3d8.dll';$dependencies=Join-Path $build 'dependencies.txt'
# Static CRT avoids a hidden dependency on a developer machine's installed Visual C++ redistributable.
$compile='cl /nologo /LD /MT /O2 /EHsc /std:c++17 /DNDEBUG /DD3D8TO9NOLOG /DNOMINMAX '+($sources -join ' ')+
    ' /link /DEF:"'+(Join-Path $source 'res/d3d8.def')+'" /OUT:"'+$dll+'" d3d9.lib user32.lib gdi32.lib shell32.lib'
$script=Join-Path $build 'compile.cmd'
[IO.File]::WriteAllLines($script,@('@echo off','chcp 65001 >nul',('call "'+$environment+'" >nul'),
    'if errorlevel 1 exit /b 1',$compile,'if errorlevel 1 exit /b 1',('dumpbin /dependents "'+$dll+'" > "'+$dependencies+'"')),[Text.UTF8Encoding]::new($false))
Push-Location $build
try{& $env:COMSPEC /d /c compile.cmd;if($LASTEXITCODE){throw 'Static Direct3D bridge compilation failed.'}}
finally{Pop-Location}
if((Get-Content -LiteralPath $dependencies -Raw) -match '(?im)\b(msvcp\d+|msvcr\d+|vcruntime\d+|api-ms-win-crt[-\w]*)\.dll\b') {
    throw 'Direct3D bridge unexpectedly imports an external C/C++ runtime.'
}
@{FormatVersion=1;BridgeRevision='255338f698c8270b537f0a91a13f795f4f988250';Runtime='static-msvc';
    BridgeSha256=(Get-FileHash -LiteralPath $dll).Hash.ToLowerInvariant()} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'direct3d-runtime.json') -Encoding UTF8
Write-Output 'Built x86 d3d8to9 with static C++ runtime; import table checked.'
