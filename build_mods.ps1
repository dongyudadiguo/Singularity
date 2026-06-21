$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$srcDir = Join-Path $root 'mods_src'
$modsDir = Join-Path $root 'mods'
$mapFile = Join-Path $root 'mods_map.txt'

if (-not (Test-Path -LiteralPath $srcDir)) { throw "Missing mods_src directory" }
if (-not (Test-Path -LiteralPath $modsDir)) { New-Item -ItemType Directory -Path $modsDir | Out-Null }

$compiler = $env:CC
if ([string]::IsNullOrWhiteSpace($compiler)) { $compiler = 'gcc' }

$entries = @()

Get-ChildItem -LiteralPath $srcDir -Filter '*.c' | Sort-Object Name | ForEach-Object {
    $src = $_.FullName
    $name = $_.BaseName
    $tmp = Join-Path $modsDir "$name.tmp.dll"
    $out = Join-Path $modsDir "$name.dll"

    & $compiler -shared -O2 -o $tmp $src -lws2_32 -ladvapi32 -luser32 -lgdi32 2>&1 | Write-Host
    if ($LASTEXITCODE -ne 0) { throw "compile failed: $name" }

    $hash = Get-FileHash -Algorithm SHA256 -LiteralPath $tmp
    $hex = $hash.Hash.ToLowerInvariant()
    $final = Join-Path $modsDir "$hex.dll"

    if (Test-Path -LiteralPath $final) { Remove-Item -LiteralPath $final }
    Move-Item -LiteralPath $tmp -Destination $final

    $entries += [pscustomobject]@{ Name = $name; Hash = $hex; File = "$hex.dll" }
}

$entries | Sort-Object Name | ForEach-Object { "{0}`t{1}`t{2}" -f $_.Name, $_.Hash, $_.File } | Set-Content -LiteralPath $mapFile
