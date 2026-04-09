$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$src = Join-Path $root "src"
$data = Join-Path $root "data"
$tools = Join-Path $root "tools"
$androidDir = Join-Path $root "android"
$appDir = Join-Path $androidDir "app"
$sdkDir = if ($env:ANDROID_SDK_ROOT) {
    $env:ANDROID_SDK_ROOT
} elseif ($env:ANDROID_HOME) {
    $env:ANDROID_HOME
} else {
    Join-Path $env:LOCALAPPDATA "Android\Sdk"
}
$sevenZip = @(
    (Get-Command 7z.exe -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
    "C:\Program Files\7-Zip\7z.exe",
    "C:\Program Files (x86)\7-Zip\7z.exe"
) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
$python = (Get-Command python -ErrorAction Stop).Source
$ndkVersion = "android-ndk-r17c"
$ndkZip = Join-Path $tools "$ndkVersion-windows-x86_64.zip"
$ndkExtractRoot = Join-Path $tools $ndkVersion
$ndkDir = Join-Path $ndkExtractRoot $ndkVersion
$ndkUrl = "https://dl.google.com/android/repository/android-ndk-r17c-windows-x86_64.zip"
$toolchainArm64 = Join-Path $tools "android-toolchain-arm64"
$toolchainX64 = Join-Path $tools "android-toolchain-x86_64"
$androidAssetsRoot = Join-Path $appDir "src\main\assets\runtime\common\milena_root"
$androidJniLibs = Join-Path $appDir "src\main\jniLibs"
$vendoredPl1 = Join-Path $androidAssetsRoot "mbrola\pl1"
$vendoredArm64Dir = Join-Path $androidJniLibs "arm64-v8a"
$vendoredX64Dir = Join-Path $androidJniLibs "x86_64"
$distDir = Join-Path $root "dist\android"
$encAscii = [Text.Encoding]::ASCII
$encIso = [Text.Encoding]::GetEncoding("iso-8859-2")
$releaseVersion = "1.0.0"

function Write-Status([string]$message) {
    Write-Host "==> $message"
}

function Ensure-Path([string]$path, [string]$label) {
    if (-not (Test-Path $path)) {
        throw "$label not found: $path"
    }
}

function Ensure-GeneratedArtifacts() {
    Write-Status "Generating Milena headers and data"

    $milenaHIn = [IO.File]::ReadAllText((Join-Path $src "milena.h.in"))
    $milenaH = $milenaHIn.Replace("{VERSION}", "0.3.8").Replace("{DATAPATH}", "data").Replace("{MORFOL}", "")
    [IO.File]::WriteAllText((Join-Path $src "milena.h"), $milenaH, $encAscii)

    $morphLines = [IO.File]::ReadAllLines((Join-Path $src "morfologik\libmorfologik.h"))
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
}

function Ensure-Ndk() {
    if (Test-Path $ndkDir) {
        return
    }

    New-Item -ItemType Directory -Force $tools | Out-Null
    if (-not (Test-Path $ndkZip)) {
        Write-Status "Downloading Android NDK r17c"
        Invoke-WebRequest -Uri $ndkUrl -OutFile $ndkZip
    }

    Write-Status "Extracting Android NDK r17c"
    if ($sevenZip) {
        & $sevenZip x -y $ndkZip "-o$ndkExtractRoot" | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "Failed to extract Android NDK"
        }
    } else {
        Expand-Archive -Path $ndkZip -DestinationPath $ndkExtractRoot -Force
    }
}

function Ensure-StandaloneToolchain([string]$arch, [string]$installDir) {
    if (Test-Path (Join-Path $installDir "bin")) {
        return
    }
    Write-Status "Creating standalone toolchain for $arch"
    & $python (Join-Path $ndkDir "build\tools\make_standalone_toolchain.py") --arch $arch --api 26 --install-dir $installDir --force
    if ($LASTEXITCODE -ne 0) {
        throw "make_standalone_toolchain.py failed for $arch"
    }
}

function Build-NativeBinary([string]$gccPath, [string]$outputPath) {
    Write-Status "Building $(Split-Path $outputPath -Leaf)"
    New-Item -ItemType Directory -Force (Split-Path $outputPath -Parent) | Out-Null
    & $gccPath -Isrc -fPIE -pie -D__ANDROID_API__=26 -o $outputPath `
        (Join-Path $src "main.c") `
        (Join-Path $src "input.c") `
        (Join-Path $src "serwer.c") `
        (Join-Path $src "mod_mbrola.c") `
        (Join-Path $src "milena.c")
    if ($LASTEXITCODE -ne 0) {
        throw "Native build failed for $outputPath"
    }
}

function Sync-AndroidRuntime() {
    Write-Status "Syncing Android runtime assets"
    Ensure-Path $vendoredPl1 "Vendored MBROLA voice pl1"
    Ensure-Path (Join-Path $vendoredArm64Dir "libmbrola_exec.so") "Vendored arm64 MBROLA binary"
    Ensure-Path (Join-Path $vendoredArm64Dir "libc++_shared.so") "Vendored arm64 libc++_shared.so"
    Ensure-Path (Join-Path $vendoredX64Dir "libmbrola_exec.so") "Vendored x86_64 MBROLA binary"
    Ensure-Path (Join-Path $vendoredX64Dir "libc++_shared.so") "Vendored x86_64 libc++_shared.so"

    $dataDest = Join-Path $androidAssetsRoot "data"
    New-Item -ItemType Directory -Force $dataDest | Out-Null
    Copy-Item (Join-Path $data "*") $dataDest -Recurse -Force
}

function Ensure-LocalProperties() {
    Write-Status "Writing android/local.properties"
    Ensure-Path $sdkDir "Android SDK"
    $sdkEscaped = $sdkDir.Replace('\', '\\')
    [IO.File]::WriteAllText((Join-Path $androidDir "local.properties"), "sdk.dir=$sdkEscaped`r`n", $encAscii)
}

function Build-AndroidArtifacts() {
    Write-Status "Building Android APK and AAB"
    Push-Location $androidDir
    try {
        & ".\gradlew.bat" assembleDebug assembleRelease bundleRelease --console=plain
        if ($LASTEXITCODE -ne 0) {
            throw "Gradle build failed"
        }
    } finally {
        Pop-Location
    }
}

function Collect-Artifacts() {
    Write-Status "Collecting Android artifacts"
    New-Item -ItemType Directory -Force $distDir | Out-Null
    Copy-Item (Join-Path $androidDir "app\build\outputs\apk\debug\app-debug.apk") (Join-Path $distDir "MilenaAndroid-$releaseVersion-debug.apk") -Force
    Copy-Item (Join-Path $androidDir "app\build\outputs\apk\release\app-release.apk") (Join-Path $distDir "MilenaAndroid-$releaseVersion-release.apk") -Force
    Copy-Item (Join-Path $androidDir "app\build\outputs\bundle\release\app-release.aab") (Join-Path $distDir "MilenaAndroid-$releaseVersion-release.aab") -Force
}

Ensure-GeneratedArtifacts
Ensure-Ndk
Ensure-StandaloneToolchain "arm64" $toolchainArm64
Ensure-StandaloneToolchain "x86_64" $toolchainX64
Build-NativeBinary (Join-Path $toolchainArm64 "bin\aarch64-linux-android-gcc.exe") (Join-Path $androidJniLibs "arm64-v8a\libmilena_exec.so")
Build-NativeBinary (Join-Path $toolchainX64 "bin\x86_64-linux-android-gcc.exe") (Join-Path $androidJniLibs "x86_64\libmilena_exec.so")
Sync-AndroidRuntime
Ensure-LocalProperties
Build-AndroidArtifacts
Collect-Artifacts

Write-Status "Done"
Write-Host "APK debug: $(Join-Path $distDir "MilenaAndroid-$releaseVersion-debug.apk")"
Write-Host "APK release: $(Join-Path $distDir "MilenaAndroid-$releaseVersion-release.apk")"
Write-Host "AAB release: $(Join-Path $distDir "MilenaAndroid-$releaseVersion-release.aab")"
