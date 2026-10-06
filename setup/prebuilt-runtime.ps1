# Player packages contain only our two x86 modules and their notices, never retail Warcraft libraries.
function Assert-OrdinaryRuntimePath([string]$Path) {
    for ($part=[IO.Path]::GetFullPath($Path);$part;$part=[IO.Path]::GetDirectoryName($part)) {
        if ((Test-Path -LiteralPath $part) -and ((Get-Item -LiteralPath $part -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
            throw 'Runtime packages must not be extracted or installed through junctions.'
        }
    }
}
function Assert-X86Module([byte[]]$Bytes) {
    if ($Bytes.Length -lt 256 -or $Bytes[0] -ne 77 -or $Bytes[1] -ne 90) { throw 'Invalid prebuilt Windows module.' }
    $pe=[BitConverter]::ToInt32($Bytes,60)
    if ($pe -lt 64 -or $pe -gt $Bytes.Length-26 -or [BitConverter]::ToUInt32($Bytes,$pe) -ne 17744 -or
        [BitConverter]::ToUInt16($Bytes,$pe+4) -ne 332 -or !([BitConverter]::ToUInt16($Bytes,$pe+22) -band 8192)) {
        throw 'Prebuilt modules must be x86 DLLs for Warcraft III 1.26a.'
    }
}
function Get-RuntimeHash([byte[]]$Bytes) {
    $sha=[Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($Bytes))).Replace('-','').ToLowerInvariant() } finally { $sha.Dispose() }
}
function Read-RuntimePackage([byte[]]$Bytes,[string]$Revision,[string]$Version) {
    Add-Type -AssemblyName System.IO.Compression
    $memory=[IO.MemoryStream]::new($Bytes,$false)
    $zip=[IO.Compression.ZipArchive]::new($memory,[IO.Compression.ZipArchiveMode]::Read)
    try {
        $allowed=@('runtime.json','WarcraftCS.mix','Mss32.dll','NOTICE','LICENSE-MIT','LICENSE-APACHE',
            'licenses/MinHook-BSD-2-Clause.txt','licenses/ReGameDLL_CS-MIT.txt','licenses/ReHLDS-MIT.txt')
        if ($zip.Entries.Count -ne $allowed.Count) { throw 'Runtime package has unexpected or missing files.' }
        $files=@{};$total=0L
        # Validate the complete fixed layout and size before extracting anything onto disk.
        foreach ($entry in $zip.Entries) {
            if ($entry.FullName -cnotin $allowed -or $files.ContainsKey($entry.FullName)) { throw 'Unsafe or duplicate runtime package entry.' }
            $total+=$entry.Length
            if ($total -gt 5000000) { throw 'Runtime package is too large.' }
            $stream=$entry.Open();$content=[IO.MemoryStream]::new()
            try { $stream.CopyTo($content);$files[$entry.FullName]=$content.ToArray() } finally { $stream.Dispose();$content.Dispose() }
        }
        $manifest=[Text.Encoding]::UTF8.GetString($files['runtime.json']).TrimStart([char]65279) | ConvertFrom-Json
        if ($manifest.SourceRevision -cne $Revision -or $manifest.Version -cne $Version -or $manifest.HostVersion -cne '1.26.0.6401' -or
            $manifest.OriginalSoundSha256 -cnotmatch '^[0-9a-f]{64}$') { throw 'Runtime package does not match the source version or supported Warcraft host.' }
        foreach ($name in $allowed | Where-Object { $_ -ne 'runtime.json' }) {
            $expected=$manifest.Files.PSObject.Properties[$name].Value
            if ($expected -cnotmatch '^[0-9a-f]{64}$' -or (Get-RuntimeHash $files[$name]) -cne $expected) { throw "Runtime package checksum mismatch: $name" }
        }
        foreach ($name in @('WarcraftCS.mix','Mss32.dll')) { Assert-X86Module $files[$name] }
        return $files
    } finally { $zip.Dispose();$memory.Dispose() }
}
function Expand-LauncherRuntime([string]$LauncherFile,[string]$Source,[string]$Destination) {
    Assert-OrdinaryRuntimePath $Destination
    # Read embedded data only: no code in the candidate launcher is invoked during extraction.
    $assembly=[Reflection.Assembly]::LoadFile([IO.Path]::GetFullPath($LauncherFile))
    $resource=$assembly.GetManifestResourceStream('WarcraftCS.Runtime.zip')
    $sourceResource=$assembly.GetManifestResourceStream('WarcraftCS.Source.zip')
    if (!$resource -or !$sourceResource) {
        if ($resource) { $resource.Dispose() };if ($sourceResource) { $sourceResource.Dispose() }
        throw 'This launcher has no prebuilt Player package. Download the new launcher or explicitly choose Developer mode.'
    }
    $revision=Split-Path -Leaf $Source
    $memory=[IO.MemoryStream]::new();$sourceMemory=[IO.MemoryStream]::new()
    try {
        if ($resource.Length -gt 5000000 -or $sourceResource.Length -gt 10000000) { throw 'Embedded package is too large.' }
        $resource.CopyTo($memory);$sourceResource.CopyTo($sourceMemory)
        if ((Get-RuntimeHash $sourceMemory.ToArray()) -cne $revision) { throw 'Launcher runtime belongs to a different source revision.' }
        $files=Read-RuntimePackage $memory.ToArray() $revision ((Get-Content -LiteralPath (Join-Path $Source 'VERSION') -Raw).Trim())
        foreach ($name in $files.Keys) { Assert-OrdinaryRuntimePath (Join-Path $Destination $name) }
        New-Item -ItemType Directory -Path $Destination -Force | Out-Null
        foreach ($name in $files.Keys) {
            $output=Join-Path $Destination $name
            New-Item -ItemType Directory -Path (Split-Path -Parent $output) -Force | Out-Null
            [IO.File]::WriteAllBytes($output,$files[$name])
        }
    } finally { $resource.Dispose();$sourceResource.Dispose();$memory.Dispose();$sourceMemory.Dispose() }
}
function Assert-PrebuiltHost([string]$Directory,[string]$Warcraft) {
    $manifest=Get-Content -LiteralPath (Join-Path $Directory 'runtime.json') -Raw -Encoding UTF8 | ConvertFrom-Json
    # The generated Miles forwarders target the exact owned library used for this supported build.
    $original=Join-Path $Warcraft 'WarcraftOriginalMss.dll'
    if (!(Test-Path -LiteralPath $original)) { $original=Join-Path $Warcraft 'Mss32.dll' }
    if ((Get-FileHash -LiteralPath $original -Algorithm SHA256).Hash.ToLowerInvariant() -cne $manifest.OriginalSoundSha256) {
        throw 'Your Miles library differs from the Player package host. Use Developer mode to generate matching forwarders locally.'
    }
    foreach ($name in @('WarcraftCS.mix','Mss32.dll')) {
        $bytes=[IO.File]::ReadAllBytes((Join-Path $Directory $name));Assert-X86Module $bytes
        if ((Get-RuntimeHash $bytes) -cne $manifest.Files.PSObject.Properties[$name].Value) { throw 'Prebuilt module checksum mismatch.' }
    }
}
function Install-PrebuiltRuntime([string]$Directory,[string]$Runtime,[string]$Warcraft) {
    Assert-PrebuiltHost $Directory $Warcraft
    if (!(Test-Path -LiteralPath (Join-Path $Runtime '.warcraft-cs-private-runtime'))) { throw 'Prebuilt modules require a marked private runtime.' }
    foreach ($name in @('WarcraftCS.mix','Mss32.dll')) {
        Assert-OrdinaryRuntimePath (Join-Path $Runtime $name)
        Copy-Item -LiteralPath (Join-Path $Directory $name) -Destination (Join-Path $Runtime $name) -Force
    }
    # Keep every bundled third-party notice alongside the installed modules.
    $notices=Join-Path $Runtime 'WarcraftCS/notices';Assert-OrdinaryRuntimePath $notices
    New-Item -ItemType Directory -Path $notices -Force | Out-Null
    foreach ($name in @('NOTICE','LICENSE-MIT','LICENSE-APACHE','licenses/MinHook-BSD-2-Clause.txt','licenses/ReGameDLL_CS-MIT.txt','licenses/ReHLDS-MIT.txt')) {
        $output=Join-Path $notices ([IO.Path]::GetFileName($name));Assert-OrdinaryRuntimePath $output
        Copy-Item -LiteralPath (Join-Path $Directory $name) -Destination $output -Force
    }
}
