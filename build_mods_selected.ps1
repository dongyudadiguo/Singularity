$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$srcDir = Join-Path $root 'mods_src'
$modsDir = Join-Path $root 'mods'
$mapFile = Join-Path $root 'mods_map.txt'

if ($args.Count -eq 0) { throw 'usage: build_mods_selected.ps1 <name> [name ...]' }
if (-not (Test-Path -LiteralPath $srcDir)) { throw "Missing mods_src directory" }
if (-not (Test-Path -LiteralPath $modsDir)) { New-Item -ItemType Directory -Path $modsDir | Out-Null }

$compiler = $env:CC
if ([string]::IsNullOrWhiteSpace($compiler)) { $compiler = 'gcc' }

$existing = @{}
if (Test-Path -LiteralPath $mapFile) {
    $inConflict = $false
    $takeConflictSide = $true
    Get-Content -LiteralPath $mapFile | ForEach-Object {
        if ($_.StartsWith('<<<<<<<')) {
            $inConflict = $true
            $takeConflictSide = $true
            return
        }
        if ($inConflict -and $_.StartsWith('=======')) {
            $takeConflictSide = $false
            return
        }
        if ($inConflict -and $_.StartsWith('>>>>>>>')) {
            $inConflict = $false
            $takeConflictSide = $true
            return
        }
        if ($inConflict -and -not $takeConflictSide) { return }
        $parts = $_ -split "`t"
        if ($parts.Count -eq 3 -and -not [string]::IsNullOrWhiteSpace($parts[0])) {
            $existing[$parts[0]] = [pscustomobject]@{ Name = $parts[0]; Hash = $parts[1]; File = $parts[2] }
        }
    }
}

foreach ($name in $args) {
    if ($name -match '[\\/]') { throw "invalid module name: $name" }

    $src = Join-Path $srcDir "$name.c"
    if (-not (Test-Path -LiteralPath $src)) { throw "missing source: $src" }

    $tmp = Join-Path $modsDir "$name.tmp.dll"
    & $compiler -shared -O2 -o $tmp $src -lws2_32 -ladvapi32 -luser32 -lgdi32 2>&1 | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "compile failed: $name" }

    $hex = (Get-FileHash -Algorithm SHA256 -LiteralPath $tmp).Hash.ToLowerInvariant()
    $final = Join-Path $modsDir "$hex.dll"

    if (Test-Path -LiteralPath $final) { Remove-Item -LiteralPath $final }
    Move-Item -LiteralPath $tmp -Destination $final

    $existing[$name] = [pscustomobject]@{ Name = $name; Hash = $hex; File = "$hex.dll" }
    "{0}`t{1}`t{2}" -f $name, $hex, "$hex.dll"
}

$mapLines = @($existing.Values | Sort-Object Name | ForEach-Object { "{0}`t{1}`t{2}" -f $_.Name, $_.Hash, $_.File })
$tmpMap = "$mapFile.tmp"
[System.IO.File]::WriteAllLines($tmpMap, $mapLines)
Move-Item -LiteralPath $tmpMap -Destination $mapFile -Force
