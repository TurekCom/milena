param(
    [string]$MbrolaExeSource = 'C:\Program Files\Festival Polski MBROLA SAPI5\festival\mbrola\mbrola.exe',
    [string]$MbrolaVoiceSource = 'C:\Program Files\Festival Polski MBROLA SAPI5\festival\mbrola\pl1'
)

$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$distDir = Join-Path $root 'sapi5_milena\dist'
$buildX64 = Join-Path $root 'sapi5_milena\build-x64'
$buildX86 = Join-Path $root 'sapi5_milena\build-x86'

function Get-BuiltDllPath {
    param(
        [Parameter(Mandatory = $true)][string]$BuildDir
    )

    $candidate = Join-Path $BuildDir 'Release\MilenaSapi5.dll'
    if (Test-Path $candidate) {
        return $candidate
    }

    $found = Get-ChildItem -Path $BuildDir -Recurse -Filter 'MilenaSapi5.dll' |
        Where-Object { $_.FullName -match '\\Release\\' } |
        Select-Object -First 1
    if (-not $found) {
        throw "Nie znaleziono zbudowanej biblioteki w: $BuildDir"
    }
    return $found.FullName
}

& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $root 'build_windows.ps1')

New-Item -ItemType Directory -Force -Path (Join-Path $distDir 'x64') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $distDir 'x86') | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $distDir 'mbrola') | Out-Null

Copy-Item (Join-Path $root 'milena.exe') (Join-Path $distDir 'milena.exe') -Force
Copy-Item (Join-Path $root 'data') (Join-Path $distDir 'data') -Recurse -Force
if (!(Test-Path $MbrolaExeSource)) {
    throw "Nie znaleziono źródłowego mbrola.exe: $MbrolaExeSource"
}
if (!(Test-Path $MbrolaVoiceSource)) {
    throw "Nie znaleziono źródłowego głosu MBROLA: $MbrolaVoiceSource"
}
Copy-Item $MbrolaExeSource (Join-Path $distDir 'mbrola\mbrola.exe') -Force
Copy-Item $MbrolaVoiceSource (Join-Path $distDir 'mbrola\pl1') -Force

& cmake -S (Join-Path $root 'sapi5_milena') -B $buildX64 -G 'Visual Studio 17 2022' -A x64
& cmake --build $buildX64 --config Release
& cmake -S (Join-Path $root 'sapi5_milena') -B $buildX86 -G 'Visual Studio 17 2022' -A Win32
& cmake --build $buildX86 --config Release

Copy-Item (Get-BuiltDllPath -BuildDir $buildX64) (Join-Path $distDir 'x64\MilenaSapi5.dll') -Force
try {
    Copy-Item (Get-BuiltDllPath -BuildDir $buildX86) (Join-Path $distDir 'x86\MilenaSapi5.dll') -Force
} catch {
    Write-Warning "Nie udało się podmienić DLL x86 w stagingu: $($_.Exception.Message)"
}

[pscustomobject]@{
    DistDir = $distDir
    MilenaExe = (Join-Path $distDir 'milena.exe')
    MbrolaExe = (Join-Path $distDir 'mbrola\mbrola.exe')
    MbrolaVoice = (Join-Path $distDir 'mbrola\pl1')
    X64Dll = (Join-Path $distDir 'x64\MilenaSapi5.dll')
    X86Dll = (Join-Path $distDir 'x86\MilenaSapi5.dll')
}
