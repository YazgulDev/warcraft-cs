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
Require-Rejected { Expand-LauncherRuntime $Launcher (Join-Path $work ('b'*64)) (Join-Path $work 'wrong') } 'Launcher/source mismatch accepted.'
if (Test-Path -LiteralPath (Join-Path $work 'wrong')) { throw 'Invalid runtime wrote files.' }
Write-Output 'PASS Player payload: embedded modules, source/version binding, hashes, x86 DLLs, fixed layout, traversal and corrupt-package rejection.'
