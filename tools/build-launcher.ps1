param([string]$OutputDirectory='',[string]$WarcraftDirectory='',[switch]$SourceOnly)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot 'paths.ps1')
if (!$OutputDirectory) { $OutputDirectory=Join-Path $root 'dist' }
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
if (!(Test-Path -LiteralPath $compiler)) { throw 'The Windows .NET Framework C# compiler is required to build the launcher.' }
$build=Join-Path $root 'build/launcher';New-Item -ItemType Directory -Path $build -Force | Out-Null
New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null
$python=Get-WarcraftCsPython $root
# Embed only the audited source tree. Retail files, downloaded dependencies and binaries never enter the source payload.
& $python (Join-Path $root 'tools/audit_sources.py') --staged
if ($LASTEXITCODE) { throw 'Stage and audit the complete launcher source before building.' }
$tree=& git -c "safe.directory=$($root.Replace('\','/'))" -C $root write-tree
if ($LASTEXITCODE) { throw 'Could not snapshot staged sources.' }
$payload=Join-Path $build 'Source.zip'
& git -c "safe.directory=$($root.Replace('\','/'))" -C $root archive --format=zip "--output=$payload" $tree
if ($LASTEXITCODE) { throw 'Could not create source payload.' }
$version=(Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
# Require the same compact English change-list contract that the launcher accepts at runtime.
& git -c "safe.directory=$($root.Replace('\','/'))" -C $root diff --quiet -- launcher docs/launcher-changes tools/build-launcher.ps1
if ($LASTEXITCODE) { throw 'Stage launcher sources and reviewed change lists before building.' }
$changesFile=Join-Path $root ('docs/launcher-changes/'+$version+'.txt')
if (!(Test-Path -LiteralPath $changesFile)) { throw 'Add a reviewed launcher change list for this version before building.' }
$changes=(Get-Content -LiteralPath $changesFile -Raw -Encoding UTF8).Trim()
if (!$changes -or @($changes -split '\r?\n' | Where-Object { $_ -notmatch '^- (Added|Fixed|Changed|Updated|Improved|Removed|Enabled|Disabled|Preserved)\s+.+' }).Count -or
    [regex]::IsMatch($changes,'[\p{L}-[A-Za-z]]|[<>`]|[\x00-\x08\x0b\x0c\x0e-\x1f]')) {
    throw 'Each launcher change must be one plain English line beginning with - Added, - Fixed, - Changed or another supported change verb.'
}
$notesResources=@(Get-ChildItem -LiteralPath (Join-Path $root 'docs/launcher-changes') -Filter '*.txt' -File | ForEach-Object {
    '/resource:'+ $_.FullName+',WarcraftCS.ReleaseNotes.'+$_.Name
})
$assembly=Join-Path $build 'LauncherVersion.cs'
('[assembly: System.Reflection.AssemblyTitle("Warcraft CS by Yazgul Launcher")]'+[Environment]::NewLine+
 '[assembly: System.Reflection.AssemblyVersion("'+$version+'.0")]') | Set-Content -LiteralPath $assembly -Encoding utf8
$sources=@(Get-ChildItem -LiteralPath (Join-Path $root 'launcher') -Filter '*.cs' -File -Recurse | ForEach-Object { $_.FullName })
$exe=Join-Path $OutputDirectory 'WarcraftCSLauncher.exe'
# The primary executable and every public ZIP contain reviewed sources without prebuilt native modules.
$references=@('/r:System.Windows.Forms.dll','/r:System.Drawing.dll','/r:System.IO.Compression.dll','/r:System.IO.Compression.FileSystem.dll','/r:System.Web.Extensions.dll')
& $compiler /nologo /target:winexe /platform:x64 /optimize+ "/out:$exe" "/resource:$payload,WarcraftCS.Source.zip" $notesResources $references $assembly $sources
if ($LASTEXITCODE) { throw 'Launcher compilation failed.' }
Get-Item -LiteralPath $exe | Select-Object FullName,Length
Get-FileHash -LiteralPath $exe -Algorithm SHA256 | Select-Object Hash
# Publish the exact embedded source snapshot and a manifest last, after its packages are available.
$sourceArchive=Join-Path $OutputDirectory 'WarcraftCS-sources.zip'
Copy-Item -LiteralPath $payload -Destination $sourceArchive -Force
$sourceHash=(Get-FileHash -LiteralPath $sourceArchive -Algorithm SHA256).Hash.ToLowerInvariant()
$launcherHash=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
$manifest=@{Version=$version;ReleaseNotes=$changes;Revision=$sourceHash;SourceSha256=$sourceHash;LauncherSha256=$launcherHash}
if (!$SourceOnly) {
    # Keep the native ZIP in private build output; only the separately named EXE distributes this payload.
    & git -c "safe.directory=$($root.Replace('\','/'))" -C $root diff --quiet -- src tools/build.ps1 tools/build-runtime-package.ps1 config NOTICE licenses LICENSE-MIT LICENSE-APACHE
    if ($LASTEXITCODE) { throw 'Stage native sources and notices before packaging Player modules.' }
    $runtimeArchive=Join-Path $build 'WarcraftCS-runtime.zip'
    & (Join-Path $PSScriptRoot 'build-runtime-package.ps1') -SourceRevision $sourceHash -OutputFile $runtimeArchive -WarcraftDirectory $WarcraftDirectory
    if ($LASTEXITCODE) { throw 'Runtime packaging failed.' }
    $included=Join-Path $OutputDirectory 'WarcraftCSLauncher_DLL_Included.exe'
    & $compiler /nologo /target:winexe /platform:x64 /optimize+ "/out:$included" "/resource:$payload,WarcraftCS.Source.zip" "/resource:$runtimeArchive,WarcraftCS.Runtime.zip" $notesResources $references $assembly $sources
    if ($LASTEXITCODE) { throw 'DLL-included launcher compilation failed.' }
    $manifest.DllIncludedLauncherSha256=(Get-FileHash -LiteralPath $included -Algorithm SHA256).Hash.ToLowerInvariant()
    $manifest.DllIncludedRuntimeSha256=(Get-FileHash -LiteralPath $runtimeArchive -Algorithm SHA256).Hash.ToLowerInvariant()
    ($manifest.DllIncludedLauncherSha256+'  WarcraftCSLauncher_DLL_Included.exe') | Set-Content -LiteralPath ($included+'.sha256') -Encoding ascii
}
# Legacy update readers see a source-build release; new readers choose their explicitly hashed EXE variant.
$manifest |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $OutputDirectory 'WarcraftCS-update.json') -Encoding UTF8
($launcherHash+'  WarcraftCSLauncher.exe') | Set-Content -LiteralPath ($exe+'.sha256') -Encoding ascii
$distribution=Join-Path $OutputDirectory ('WarcraftCS-'+$version+'-dist.zip')
if (Test-Path -LiteralPath $distribution) { Remove-Item -LiteralPath $distribution -Force }
# Let players read all prerequisites before launching setup or extracting the source archive.
$requirements=Join-Path $OutputDirectory 'REQUIREMENTS.md'
Copy-Item -LiteralPath (Join-Path $root 'REQUIREMENTS.md') -Destination $requirements -Force
Compress-Archive -LiteralPath $exe,($exe+'.sha256'),$sourceArchive,(Join-Path $OutputDirectory 'WarcraftCS-update.json'),$requirements -DestinationPath $distribution
