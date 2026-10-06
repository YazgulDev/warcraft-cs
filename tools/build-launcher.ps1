param([string]$OutputDirectory='')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'paths.ps1')
if (!$OutputDirectory) { $OutputDirectory=Join-Path $root 'dist' }
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
if (!(Test-Path -LiteralPath $compiler)) { throw 'The Windows .NET Framework C# compiler is required to build the launcher.' }
$build=Join-Path $root 'build/launcher';New-Item -ItemType Directory -Path $build -Force | Out-Null
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$python=Get-WarcraftCsPython $root
# Embed only the audited source tree. Retail files, downloaded dependencies and generated binaries never enter the EXE.
& $python (Join-Path $root 'tools/audit_sources.py') --staged
if ($LASTEXITCODE) { throw 'Stage and audit the complete launcher source before building.' }
$tree=& git -c "safe.directory=$($root.Replace('\','/'))" -C $root write-tree
if ($LASTEXITCODE) { throw 'Could not snapshot staged sources.' }
$payload=Join-Path $build 'Source.zip'
& git -c "safe.directory=$($root.Replace('\','/'))" -C $root archive --format=zip "--output=$payload" $tree
if ($LASTEXITCODE) { throw 'Could not create source payload.' }
$version=(Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
$assembly=Join-Path $build 'LauncherVersion.cs'
('[assembly: System.Reflection.AssemblyTitle("Warcraft CS by Yazgul Launcher")]'+[Environment]::NewLine+
 '[assembly: System.Reflection.AssemblyVersion("'+$version+'.0")]') | Set-Content -LiteralPath $assembly -Encoding utf8
$sources=@(Get-ChildItem -LiteralPath (Join-Path $root 'launcher') -Filter '*.cs' -File | ForEach-Object { $_.FullName })
$exe=Join-Path $OutputDirectory 'WarcraftCSLauncher.exe'
& $compiler /nologo /target:winexe /platform:x64 /optimize+ "/out:$exe" "/resource:$payload,WarcraftCS.Source.zip" /r:System.Windows.Forms.dll /r:System.Drawing.dll /r:System.IO.Compression.dll /r:System.IO.Compression.FileSystem.dll /r:System.Web.Extensions.dll $assembly $sources
if ($LASTEXITCODE) { throw 'Launcher compilation failed.' }
Get-Item -LiteralPath $exe | Select-Object FullName,Length
Get-FileHash -LiteralPath $exe -Algorithm SHA256 | Select-Object Hash
# Publish the exact embedded source snapshot and a manifest last, after its packages are available.
$sourceArchive=Join-Path $OutputDirectory 'WarcraftCS-sources.zip'
Copy-Item -LiteralPath $payload -Destination $sourceArchive -Force
$sourceHash=(Get-FileHash -LiteralPath $sourceArchive -Algorithm SHA256).Hash.ToLowerInvariant()
$launcherHash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
@{Version=$version;Revision=$sourceHash;SourceSha256=$sourceHash;LauncherSha256=$launcherHash} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'WarcraftCS-update.json') -Encoding UTF8
($launcherHash+'  WarcraftCSLauncher.exe') | Set-Content -LiteralPath ($exe+'.sha256') -Encoding ascii
$distribution=Join-Path $OutputDirectory ('WarcraftCS-'+$version+'-dist.zip')
if (Test-Path -LiteralPath $distribution) { Remove-Item -LiteralPath $distribution -Force }
Compress-Archive -LiteralPath $exe,($exe+'.sha256'),$sourceArchive,(Join-Path $OutputDirectory 'WarcraftCS-update.json') -DestinationPath $distribution
