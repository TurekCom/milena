$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$distDir = Join-Path $root "dist"
$releaseDir = Join-Path $distDir "release"
$version = "1.0.0"
$checksumFile = Join-Path $releaseDir "Milena-$version-SHA256SUMS.txt"

$assets = @(
    Join-Path $distDir "installer\MilenaMBROLA-SAPI5-$version-x64.exe"
    Join-Path $distDir "nvda\Milena_MBROLA-$version.nvda-addon"
    Join-Path $distDir "android\MilenaAndroid-$version-release.apk"
    Join-Path $distDir "android\MilenaAndroid-$version-release.aab"
)

foreach ($asset in $assets) {
    if (-not (Test-Path $asset)) {
        throw "Brak artefaktu releasowego: $asset"
    }
}

New-Item -ItemType Directory -Force $releaseDir | Out-Null

$lines = foreach ($asset in $assets) {
    $hash = (Get-FileHash -Algorithm SHA256 $asset).Hash.ToLowerInvariant()
    "$hash *$(Split-Path $asset -Leaf)"
}

[IO.File]::WriteAllLines($checksumFile, $lines, [Text.Encoding]::ASCII)
Write-Host $checksumFile
