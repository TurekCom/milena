param()

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$isccCandidates = @(
    (Get-Command ISCC.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
    'C:\Users\turek\AppData\Local\UniGetUI\Chocolatey\bin\ISCC.exe',
    'C:\Program Files (x86)\Inno Setup 6\ISCC.exe',
    'C:\Program Files\Inno Setup 6\ISCC.exe'
) | Where-Object { $_ -and (Test-Path $_) }

if (-not $isccCandidates) {
    throw 'Nie znaleziono ISCC.exe (Inno Setup).'
}

& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'build_milena_sapi5.ps1')
& $isccCandidates[0] (Join-Path $root 'installer\milena_sapi5_x64.iss')
