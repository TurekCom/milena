$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$src = Join-Path $root "src"
$data = Join-Path $root "data"
$gcc = Get-Command gcc -ErrorAction Stop
$encAscii = [Text.Encoding]::ASCII
$encIso = [Text.Encoding]::GetEncoding("iso-8859-2")

Write-Host "Generating Windows build artifacts..."

$milenaHIn = [IO.File]::ReadAllText((Join-Path $src "milena.h.in"))
$milenaH = $milenaHIn.Replace("{VERSION}", "0.3.8").Replace("{DATAPATH}", "data").Replace("{MORFOL}", "")
[IO.File]::WriteAllText((Join-Path $src "milena.h"), $milenaH, $encAscii)

$morphLines = [IO.File]::ReadAllLines((Join-Path $src "morfologik\\libmorfologik.h"))
$morphOut = New-Object System.Collections.Generic.List[string]
$morphOut.Add("#ifndef def_morfologik_h")
$morphOut.Add("#define def_morfologik_h")
$morphOut.Add("#define WM_m (WM_m1 | WM_m2 | WM_m3)")
$morphOut.Add("#define WM_n (WM_n1 | WM_n2)")
foreach ($line in $morphLines) {
    if ($line -match '^#define W') {
        $morphOut.Add($line)
    }
}
$morphOut.Add("#endif")
[IO.File]::WriteAllLines((Join-Path $src "def_morfologik.h"), $morphOut, $encAscii)

$phrBytes1 = [IO.File]::ReadAllBytes((Join-Path $data "pl_phraser.dat.in"))
$phrBytes2 = [IO.File]::ReadAllBytes((Join-Path $data "pl_imiona.in"))
$phrBytes = New-Object byte[] ($phrBytes1.Length + $phrBytes2.Length)
[Array]::Copy($phrBytes1, 0, $phrBytes, 0, $phrBytes1.Length)
[Array]::Copy($phrBytes2, 0, $phrBytes, $phrBytes1.Length, $phrBytes2.Length)
[IO.File]::WriteAllBytes((Join-Path $data "pl_phraser.dat"), $phrBytes)

$cyrTemplate = [IO.File]::ReadAllText((Join-Path $src "milena_cyrillic.in"))
$cyrRules = [IO.File]::ReadAllLines((Join-Path $src "milena_cyrillic.rules"))
$renderedRules = New-Object System.Collections.Generic.List[string]
foreach ($raw in $cyrRules) {
    $line = $raw.Trim()
    if ([string]::IsNullOrWhiteSpace($line)) {
        continue
    }

    $parts = $line -split '\s+', 2
    $cyr = $parts[0]
    $lat = if ($parts.Length -gt 1) { $parts[1] } else { "" }

    $chars = $cyr.ToCharArray()
    $z1 = [int][char]$chars[0]
    $z2 = if ($chars.Length -gt 1) { [int][char]$chars[1] } else { 0 }

    $bytes = $encIso.GetBytes($lat)
    $literal = New-Object System.Text.StringBuilder
    foreach ($b in $bytes) {
        if ($b -eq 34) {
            [void]$literal.Append('\"')
        } elseif ($b -eq 92) {
            [void]$literal.Append('\\')
        } elseif ($b -ge 32 -and $b -le 126) {
            [void]$literal.Append([char]$b)
        } else {
            [void]$literal.AppendFormat('\{0:D3}', [int]$b)
        }
    }

    $renderedRules.Add(('{{0x{0:x},0x{1:x},"{2}"}}' -f $z1, $z2, $literal.ToString()))
}

$cyrHeader = $cyrTemplate.Replace("%CRULES%", [string]::Join(",`r`n", $renderedRules))
[IO.File]::WriteAllText((Join-Path $src "milena_cyrillic.h"), $cyrHeader, $encAscii)

Write-Host "Building milena.exe..."
& $gcc.Source -Isrc -o milena.exe src\main.c src\input.c src\my_stdio.c src\mod_mbrola.c src\milena.c -luser32
if ($LASTEXITCODE -ne 0) {
    throw "gcc failed with exit code $LASTEXITCODE"
}

Write-Host "Done: $root\\milena.exe"
