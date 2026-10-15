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
    $oldSection='';$customStride=$false
    foreach($existing in $lines) {
        if($existing -match '^\s*\[([^\]]+)\]\s*$'){$oldSection=$Matches[1];continue}
        if($oldSection -eq 'ThirdPerson' -and $existing -match '^\s*RunCycleDistance\s*=\s*(\d+(?:\.\d+)?)\s*$') {
            $stride=[double]::Parse($Matches[1],[Globalization.CultureInfo]::InvariantCulture)
            if($stride -ge 40 -and $stride -le 1000 -and $stride -ne 210){$customStride=$true}
        }
    }
    # Only the old default changes: F7 is now TPS entry. Preserve custom refill keys and the rest of the layout.
    $shortcutSection=$false;$refillIndex=-1;$thirdBinding=$false;$unitViewBinding=$false;$allWeaponsIndex=-1
    $usedShortcuts=[Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    for($i=0;$i -lt $lines.Count;$i++) {
        if($lines[$i] -match '^\s*\[([^\]]+)\]\s*$'){$shortcutSection=$Matches[1] -eq 'Shortcuts';continue}
        if($shortcutSection -and $lines[$i] -match '^\s*([^;#=]+?)\s*=\s*(.*?)\s*$') {
            $name=$Matches[1].Trim();$value=$Matches[2].Trim();[void]$usedShortcuts.Add($value)
            if($name -eq 'ToggleThirdPerson'){$thirdBinding=$true}
            if($name -eq 'ToggleUnitView'){$unitViewBinding=$true}
            if($name -eq 'AllWeapons' -and $value -eq 'F9'){$allWeaponsIndex=$i}
            if($name -eq 'Refill' -and $value -eq 'F7'){$refillIndex=$i}
        }
    }
    if(!$thirdBinding -and $refillIndex -ge 0) {
        $replacement='NONE'
        foreach($candidate in @('F11','F12','F5','F4')) {if(!$usedShortcuts.Contains($candidate)){$replacement=$candidate;break}}
        $lines[$refillIndex]='Refill='+$replacement;$changed=$true
        # A later F9 migration must see the newly reserved refill key, rather than assign it twice.
        [void]$usedShortcuts.Remove('F7');[void]$usedShortcuts.Add($replacement)
    }
    # F9 is the requested independent unit camera. Only its old all-weapons default is relocated.
    if(!$unitViewBinding -and $allWeaponsIndex -ge 0) {
        $replacement='NONE'
        foreach($candidate in @('F12','F5','F4')){if(!$usedShortcuts.Contains($candidate)){$replacement=$candidate;break}}
        $lines[$allWeaponsIndex]='AllWeapons='+$replacement;$changed=$true
        [void]$usedShortcuts.Remove('F9');[void]$usedShortcuts.Add($replacement)
    }
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
                    # Preserve an existing chosen manual stride rather than hiding it behind the new automatic default.
                    if($entry.name -eq 'ThirdPerson' -and $Matches[1].Trim() -eq 'AutoAnimationSpeed' -and $customStride){$line='AutoAnimationSpeed=false'}
                    # Adding Q must not conflict with a custom action that already uses it.
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'ToggleAllyTargeting' -and $usedShortcuts.Contains('Q')){$line='ToggleAllyTargeting=NONE'}
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'ToggleUnitView' -and $usedShortcuts.Contains('F9')){$line='ToggleUnitView=NONE'}
                    # A migrated/custom refill may already own the new F12 default in a partial old layout.
                    if($entry.name -eq 'Shortcuts' -and $Matches[1].Trim() -eq 'AllWeapons' -and $usedShortcuts.Contains('F12')) {
                        $replacement='NONE'
                        foreach($candidate in @('F5','F4')){if(!$usedShortcuts.Contains($candidate)){$replacement=$candidate;break}}
                        $line='AllWeapons='+$replacement
                    }
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
