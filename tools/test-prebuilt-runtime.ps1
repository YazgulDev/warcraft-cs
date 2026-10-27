param([Parameter(Mandatory=$true)][string]$Package,[Parameter(Mandatory=$true)][string]$SourceRevision,[Parameter(Mandatory=$true)][string]$Launcher)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/prebuilt-runtime.ps1')
function Require-Rejected([scriptblock]$Action,[string]$Message) {
    $rejected=$false;try { & $Action | Out-Null } catch { $rejected=$true }
    if (!$rejected) { throw $Message }
}
$version=(Get-Content -LiteralPath (Join-Path $root 'VERSION') -Raw).Trim()
$bytes=[IO.File]::ReadAllBytes([IO.Path]::GetFullPath($Package))
$files=Read-RuntimePackage $bytes $SourceRevision $version
Require-Rejected { Read-RuntimePackage $bytes ('a'*64) $version } 'Wrong source revision accepted.'
Require-Rejected { Read-RuntimePackage $bytes $SourceRevision '9.9.9' } 'Wrong version accepted.'
# Repackage verified original project files with adversarial entries; no retail file is used in this test.
function Fixture([string]$Change) {
    $memory=[IO.MemoryStream]::new()
    $zip=[IO.Compression.ZipArchive]::new($memory,[IO.Compression.ZipArchiveMode]::Create,$true)
    foreach ($name in $files.Keys) {
        $path=if ($Change -eq 'traversal' -and $name -eq 'NOTICE') { '../NOTICE' } else { $name }
        $data=$files[$name]
        if ($Change -eq 'corrupt' -and $name -eq 'WarcraftCS.mix') { $data=[byte[]]$data.Clone();$data[$data.Length-1]=$data[$data.Length-1] -bxor 1 }
        $stream=$zip.CreateEntry($path).Open();try { $stream.Write($data,0,$data.Length) } finally { $stream.Dispose() }
    }
    $zip.Dispose();$result=$memory.ToArray();$memory.Dispose();return ,$result
}
Require-Rejected { Read-RuntimePackage (Fixture 'traversal') $SourceRevision $version } 'Runtime traversal accepted.'
Require-Rejected { Read-RuntimePackage (Fixture 'corrupt') $SourceRevision $version } 'Corrupt module accepted.'
$work=Join-Path $root ('build/prebuilt-tests-'+[guid]::NewGuid().ToString('N'))
$source=Join-Path $work $SourceRevision
New-Item -ItemType Directory -Path $source -Force | Out-Null
Set-Content -LiteralPath (Join-Path $source 'VERSION') -Value $version
$destination=Join-Path $work 'modules'
Expand-LauncherRuntime $Launcher $source $destination
foreach ($name in @('WarcraftCS.mix','Mss32.dll')) {
    if ((Get-FileHash -LiteralPath (Join-Path $destination $name) -Algorithm SHA256).Hash.ToLowerInvariant() -ne (Get-RuntimeHash $files[$name])) { throw 'Embedded runtime differs from packaged module.' }
}
# Missing game identity is a different rejection from a legacy Miles fingerprint mismatch.
$badHost=Join-Path $work 'different-host';New-Item -ItemType Directory -Path $badHost -Force | Out-Null
Set-Content -LiteralPath (Join-Path $badHost 'Mss32.dll') -Value 'synthetic incompatible host'
$actual=(Get-FileHash -LiteralPath (Join-Path $badHost 'Mss32.dll') -Algorithm SHA256).Hash.ToLowerInvariant()
Require-Rejected {Assert-PrebuiltHost $destination $badHost} 'A host without Game.dll was accepted.'
# Version-only owned PE data reaches the Miles policy without using any retail game file.
$manifestPath=Join-Path $destination 'runtime.json'
$originalManifest=[IO.File]::ReadAllText($manifestPath)
$metadata=$originalManifest | ConvertFrom-Json
$fixtureSource=Join-Path $work 'HostVersion.cs'
('[assembly: System.Reflection.AssemblyFileVersion("'+$metadata.HostVersion+'")] public sealed class HostVersion {}') | Set-Content -LiteralPath $fixtureSource -Encoding ascii
$compiler=Join-Path $env:WINDIR 'Microsoft.NET/Framework64/v4.0.30319/csc.exe'
& $compiler /nologo /target:library /platform:x86 ('/out:'+(Join-Path $badHost 'Game.dll')) $fixtureSource
if($LASTEXITCODE){throw 'Synthetic host version fixture compilation failed.'}
if($metadata.FormatVersion -eq 2) {
    # Adaptive packages report both fingerprints but defer export validation to local proxy preparation.
    $diagnostic=(Assert-PrebuiltHost $destination $badHost) -join "`n"
    if(!$diagnostic.Contains($actual) -or !$diagnostic.Contains($metadata.OriginalSoundSha256)){throw 'Adaptive host check omitted diagnostic fingerprints.'}
    $runtime=Join-Path $work 'bad-export-runtime';New-Item -ItemType Directory -Path $runtime -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $runtime '.warcraft-cs-private-runtime') -Value 'synthetic fixture'
    Require-Rejected {Install-PrebuiltRuntime $destination $runtime $badHost} 'Malformed local Miles exports were accepted.'
    foreach($name in @('WarcraftCS.mix','Mss32.dll')) {if(Test-Path (Join-Path $runtime $name)){throw 'Invalid Miles exports installed a partial module.'}}
}
# Older exact-host packages must still reject a different Miles library with both hashes in the reason.
$metadata.PSObject.Properties.Remove('FormatVersion')
try {
    $metadata | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $manifestPath -Encoding UTF8
    try {Assert-PrebuiltHost $destination $badHost | Out-Null;throw 'Unexpectedly accepted incompatible legacy host.'}
    catch {if(!$_.Exception.Message.Contains($actual) -or !$_.Exception.Message.Contains('Expected')){throw 'Legacy host rejection omitted diagnostic fingerprints.'}}
} finally {[IO.File]::WriteAllText($manifestPath,$originalManifest,[Text.UTF8Encoding]::new($false))}
Require-Rejected { Expand-LauncherRuntime $Launcher (Join-Path $work ('b'*64)) (Join-Path $work 'wrong') } 'Launcher/source mismatch accepted.'
if (Test-Path -LiteralPath (Join-Path $work 'wrong')) { throw 'Invalid runtime wrote files.' }
Write-Output 'PASS Player payload: embedded modules, source/version binding, hashes, x86 DLLs, fixed layout, traversal and corrupt-package rejection.'
