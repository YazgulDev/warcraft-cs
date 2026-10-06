$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
. (Join-Path $root 'setup/audio-runtime.ps1')
$fixture=Join-Path $root ('build/audio-runtime-tests-'+[guid]::NewGuid().ToString('N'))
$owned=Join-Path $fixture 'owned'
$providers=Join-Path $owned 'redist/miles'
New-Item -ItemType Directory -Path $providers -Force | Out-Null
# Synthetic bytes exercise installation policy without game data or a native sound device.
$names=@('Mssfast.m3d','Mp3dec.asi','Reverb3.flt','Mssdolby.m3d','Msseax2.m3d')
foreach ($name in $names) { [IO.File]::WriteAllText((Join-Path $providers $name),"owned fixture $name") }
Assert-WarcraftAudioRuntime $owned
foreach ($kind in @('fresh','legacy')) {
    $runtime=Join-Path $fixture $kind
    New-Item -ItemType Directory -Path $runtime -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $runtime '.warcraft-cs-private-runtime'),'private fixture')
    $progress=Join-Path $runtime 'progress.fixture'
    [IO.File]::WriteAllText($progress,'keep saved progress and preferences')
    Copy-WarcraftAudioRuntime $owned $runtime
    Copy-WarcraftAudioRuntime $owned $runtime
    foreach ($name in $names) {
        if ([IO.File]::ReadAllText((Join-Path $runtime "redist/miles/$name")) -ne "owned fixture $name") { throw "Provider missing/corrupted: $kind $name" }
    }
    if ([IO.File]::ReadAllText($progress) -ne 'keep saved progress and preferences') { throw 'Unrelated runtime files changed' }
}
# Fail incomplete installs before writing any target files, and reject an original/unmarked destination.
$missing=Join-Path $fixture 'incomplete';New-Item -ItemType Directory -Path $missing | Out-Null
$untouched=Join-Path $fixture 'untouched'
$rejected=$false
try { Copy-WarcraftAudioRuntime $missing $untouched } catch { $rejected=$_.Exception.Message -like '*redist/miles/Mssfast.m3d*' }
if (!$rejected -or (Test-Path -LiteralPath $untouched)) { throw 'Incomplete install was accepted or wrote files' }
foreach ($target in @($owned,(Join-Path $owned 'child'),$untouched)) {
    $rejected=$false
    try { Copy-WarcraftAudioRuntime $owned $target } catch { $rejected=$true }
    if (!$rejected) { throw 'Unsafe audio destination was accepted' }
}
Write-Output 'PASS Miles providers: fresh install, legacy repair, repeat install, missing-source rejection and original-file protection.'
