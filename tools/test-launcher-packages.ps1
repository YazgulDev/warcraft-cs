param([Parameter(Mandatory=$true)][string]$Directory)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$directoryPath=[IO.Path]::GetFullPath($Directory)
$manifest=Get-Content -LiteralPath (Join-Path $directoryPath 'WarcraftCS-update.json') -Raw | ConvertFrom-Json
$standard=Join-Path $directoryPath 'WarcraftCSLauncher.exe'
$included=Join-Path $directoryPath 'WarcraftCSLauncher_DLL_Included.exe'
function Require([bool]$Condition,[string]$Message) { if (!$Condition) { throw $Message } }
# Inspect real assemblies without executing startup or allowing package inspection to install anything.
$standardAssembly=[Reflection.Assembly]::LoadFile($standard)
Require ($null -eq $standardAssembly.GetManifestResourceInfo('WarcraftCS.Runtime.zip')) 'Primary EXE contains a native runtime.'
Require ($null -ne $standardAssembly.GetManifestResourceInfo('WarcraftCS.Source.zip')) 'Primary source payload missing.'
$includedAssembly=[Reflection.Assembly]::LoadFile($included)
Require ($null -ne $includedAssembly.GetManifestResourceInfo('WarcraftCS.Runtime.zip')) 'Separate included EXE lacks its runtime.'
Require ((Get-FileHash -LiteralPath $standard).Hash -eq $manifest.LauncherSha256) 'Primary executable checksum differs.'
Require ((Get-FileHash -LiteralPath $included).Hash -eq $manifest.DllIncludedLauncherSha256) 'Included executable checksum differs.'
Require (!$manifest.RuntimeSha256) 'Primary manifest advertises DLLs to legacy clients.'
foreach ($assembly in @($standardAssembly,$includedAssembly)) {
    $stream=$assembly.GetManifestResourceStream('WarcraftCS.Source.zip');$memory=[IO.MemoryStream]::new()
    try {
        $stream.CopyTo($memory);$sha=[Security.Cryptography.SHA256]::Create()
        try { $hash=([BitConverter]::ToString($sha.ComputeHash($memory.ToArray()))).Replace('-','').ToLowerInvariant() } finally { $sha.Dispose() }
        Require ($hash -eq $manifest.SourceSha256) 'Variants embed different source snapshots.'
    } finally { $stream.Dispose();$memory.Dispose() }
}
# Every public archive must remain free of DLL/MIX modules and the included launcher/runtime ZIP.
foreach ($path in @((Join-Path $directoryPath 'WarcraftCS-sources.zip'),(Join-Path $directoryPath ('WarcraftCS-'+$manifest.Version+'-dist.zip')))) {
    $zip=[IO.Compression.ZipFile]::OpenRead($path)
    try {
        foreach ($entry in $zip.Entries) {
            Require ($entry.FullName -notmatch '(?i)\.(dll|mix)$|WarcraftCS-runtime\.zip|WarcraftCSLauncher_DLL_Included\.exe') 'Public ZIP contains a native package.'
            if ($entry.FullName -eq 'WarcraftCSLauncher.exe') {
                $stream=$entry.Open();$memory=[IO.MemoryStream]::new()
                try {
                    $stream.CopyTo($memory);$bytes=$memory.ToArray()
                    $sha=[Security.Cryptography.SHA256]::Create()
                    try { $hash=([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-','').ToLowerInvariant() } finally { $sha.Dispose() }
                    Require ($hash -eq $manifest.LauncherSha256) 'Distribution ZIP embeds the wrong launcher.'
                } finally { $stream.Dispose();$memory.Dispose() }
            }
        }
    } finally { $zip.Dispose() }
}
Write-Output 'PASS actual dual EXEs, identical source snapshots, independent hashes and native-free public ZIPs.'
