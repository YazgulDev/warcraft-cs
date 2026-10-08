# Add newly introduced settings without replacing the player's prices, controls or damage preferences.
function Update-GameplayConfig([string]$Template,[string]$Destination) {
    if (!(Test-Path -LiteralPath $Destination)) { Copy-Item -LiteralPath $Template -Destination $Destination;return }
    $defaults=[Collections.Generic.List[object]]::new()
    $section=$null
    foreach ($line in Get-Content -LiteralPath $Template) {
        if ($line -match '^\s*\[([^\]]+)\]\s*$') {
            $section=@{name=$Matches[1];lines=[Collections.Generic.List[string]]::new()}
            $defaults.Add($section)
        }
        if ($null -ne $section) { $section.lines.Add($line) }
    }
    $lines=[Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]]@(Get-Content -LiteralPath $Destination))
    $changed=$false
    foreach ($entry in $defaults) {
        $start=-1;$end=$lines.Count
        for ($i=0;$i -lt $lines.Count;$i++) {
            if ($lines[$i] -match '^\s*\[([^\]]+)\]\s*$') {
                if ($start -ge 0) { $end=$i;break }
                if ($Matches[1] -eq $entry.name) { $start=$i }
            }
        }
        if ($start -lt 0) {
            $lines.Add('');$lines.AddRange([string[]]$entry.lines);$changed=$true;continue
        }
        $keys=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        for ($i=$start+1;$i -lt $end;$i++) {
            if ($lines[$i] -match '^\s*([^;#=]+?)\s*=') { [void]$keys.Add($Matches[1].Trim()) }
        }
        $comments=[Collections.Generic.List[string]]::new()
        foreach ($line in $entry.lines) {
            if ($line -match '^\s*;') { $comments.Add($line);continue }
            if ($line -match '^\s*([^;#=]+?)\s*=') {
                # Insert into the original section, never append a duplicate INI section ignored by Win32.
                if ($keys.Add($Matches[1].Trim())) {
                    foreach ($comment in $comments) { $lines.Insert($end,$comment);$end++ }
                    $lines.Insert($end,$line);$end++;$changed=$true
                }
            }
            $comments.Clear()
        }
    }
    if ($changed) {
        # Atomic replacement keeps interrupted updates from leaving a partial settings file.
        $temporary=$Destination+'.'+[guid]::NewGuid().ToString('N')+'.tmp'
        try {
            [IO.File]::WriteAllLines($temporary,$lines,[Text.UTF8Encoding]::new($false))
            [IO.File]::Replace($temporary,$Destination,[NullString]::Value)
        } finally { if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary } }
    }
}
