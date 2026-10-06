param([Parameter(Mandatory=$true)][string]$SourceRevision,[Parameter(Mandatory=$true)][string]$OutputFile,
    [string]$WarcraftDirectory='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'paths.ps1')
if (!$WarcraftDirectory) { $WarcraftDirectory=Get-WarcraftCsRuntime $root }
$version=(Get-Item -LiteralPath (Join-Path $WarcraftDirectory 'Game.dll')).VersionInfo
if ($version.FileMajorPart -ne 1 -or $version.FileMinorPart -ne 26 -or $version.FileBuildPart -ne 0 -or $version.FilePrivatePart -ne 6401) { throw 'Build Player modules against owned Warcraft III 1.26a.' }
$original=Join-Path $WarcraftDirectory 'WarcraftOriginalMss.dll'
if (!(Test-Path -LiteralPath $original)) { $original=Join-Path $WarcraftDirectory 'Mss32.dll' }
$build=Join-Path $root 'build/player-runtime'
New-Item -ItemType Directory -Path $build -Force | Out-Null
# Only our compiler output enters the package. This owned library is a private build input, not an asset.
Copy-Item -LiteralPath $original -Destination (Join-Path $build 'WarcraftOriginalMss.dll') -Force
& (Join-Path $PSScriptRoot 'build.ps1') -OutputDirectory $build -PythonExecutable (Get-WarcraftCsPython $root)
if ($LASTEXITCODE) { throw 'Player native build failed.' }
$package=Join-Path $build 'package'
New-Item -ItemType Directory -Path (Join-Path $package 'licenses') -Force | Out-Null
$files=@('WarcraftCS.mix','Mss32.dll','NOTICE','LICENSE-MIT','LICENSE-APACHE',
    'licenses/MinHook-BSD-2-Clause.txt','licenses/ReGameDLL_CS-MIT.txt','licenses/ReHLDS-MIT.txt')
$hashes=@{}
foreach ($name in $files) {
    $source=if ($name -in @('WarcraftCS.mix','Mss32.dll')) { Join-Path $build $name } else { Join-Path $root $name }
    Copy-Item -LiteralPath $source -Destination (Join-Path $package $name) -Force
    $hashes[$name]=(Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
}
# Bind the native bundle to the exact audited source archive and supported forwarder ABI.
@{Version=(Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim();SourceRevision=$SourceRevision;
    HostVersion='1.26.0.6401';OriginalSoundSha256=(Get-FileHash -LiteralPath $original -Algorithm SHA256).Hash.ToLowerInvariant();Files=$hashes} |
    ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $package 'runtime.json') -Encoding UTF8
Add-Type -AssemblyName System.IO.Compression.FileSystem
# Reject stale package files rather than silently including any unrelated private build input.
$contents=@(Get-ChildItem -LiteralPath $package -Recurse -File | ForEach-Object { $_.FullName.Substring($package.Length+1).Replace('\','/') })
if ($contents.Count -ne $files.Count+1 -or @($contents | Where-Object { $_ -notin $files -and $_ -ne 'runtime.json' }).Count) { throw 'Unexpected file in Player package staging.' }
if (Test-Path -LiteralPath $OutputFile) { Remove-Item -LiteralPath $OutputFile -Force }
[IO.Compression.ZipFile]::CreateFromDirectory($package,[IO.Path]::GetFullPath($OutputFile))
Write-Output 'Built Player runtime: two original project modules and license notices; no retail game DLLs.'
