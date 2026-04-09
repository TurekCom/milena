param(
    [string]$InstallDir,
    [string]$MbrolaExePath = 'mbrola\mbrola.exe',
    [string]$MbrolaVoicePath = 'mbrola\pl1',
    [int]$BaseTempoPercent = 100,
    [int]$BasePitchPercent = 100,
    [switch]$MachineWide
)

$ErrorActionPreference = 'Stop'

$engineClsid = '{0E23AAFA-4F1D-4A09-A8A4-759A84EAC219}'
$tokenName = 'MilenaMbrola.Standard'
$voiceName = 'Milena - MBROLA'

if ([string]::IsNullOrWhiteSpace($InstallDir)) {
    $repoDist = Join-Path $PSScriptRoot 'sapi5_milena\dist'
    if (Test-Path (Join-Path $repoDist 'x64\MilenaSapi5.dll')) {
        $InstallDir = $repoDist
    } else {
        $InstallDir = $PSScriptRoot
    }
}
$InstallDir = [System.IO.Path]::GetFullPath($InstallDir)

$mbrolaExeTokenValue = $MbrolaExePath
$mbrolaVoiceTokenValue = $MbrolaVoicePath
if (-not [System.IO.Path]::IsPathRooted($MbrolaExePath)) {
    $MbrolaExePath = Join-Path $InstallDir $MbrolaExePath
}
if (-not [System.IO.Path]::IsPathRooted($MbrolaVoicePath)) {
    $MbrolaVoicePath = Join-Path $InstallDir $MbrolaVoicePath
}

$x64Dll = Join-Path $InstallDir 'x64\MilenaSapi5.dll'
$x86Dll = Join-Path $InstallDir 'x86\MilenaSapi5.dll'
$milenaExe = Join-Path $InstallDir 'milena.exe'
$dataDir = Join-Path $InstallDir 'data'

if (!(Test-Path $x64Dll)) { throw "Brak DLL x64: $x64Dll" }
if (!(Test-Path $x86Dll)) { throw "Brak DLL x86: $x86Dll" }
if (!(Test-Path $milenaExe)) { throw "Brak milena.exe: $milenaExe" }
if (!(Test-Path $dataDir)) { throw "Brak katalogu data: $dataDir" }
if (!(Test-Path $MbrolaExePath)) { throw "Brak mbrola.exe: $MbrolaExePath" }
if (!(Test-Path $MbrolaVoicePath)) { throw "Brak głosu MBROLA: $MbrolaVoicePath" }

function Set-ComRegistration {
    param(
        [Parameter(Mandatory = $true)][string]$BaseKey,
        [Parameter(Mandatory = $true)][string]$DllPath,
        [Parameter(Mandatory = $true)][string]$Clsid,
        [Parameter(Mandatory = $true)][string]$DisplayName
    )

    $clsidKey = Join-Path $BaseKey $Clsid
    $inprocKey = Join-Path $clsidKey 'InprocServer32'
    New-Item -Path $inprocKey -Force | Out-Null
    Set-Item -Path $clsidKey -Value $DisplayName
    Set-Item -Path $inprocKey -Value $DllPath
    New-ItemProperty -Path $inprocKey -Name 'ThreadingModel' -Value 'Both' -PropertyType String -Force | Out-Null
}

function Set-VoiceToken {
    param(
        [Parameter(Mandatory = $true)][string]$BaseKey
    )

    $tokenKey = Join-Path $BaseKey $tokenName
    $attrKey = Join-Path $tokenKey 'Attributes'
    New-Item -Path $attrKey -Force | Out-Null

    Set-Item -Path $tokenKey -Value $voiceName
    New-ItemProperty -Path $tokenKey -Name 'CLSID' -Value $engineClsid -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name '409' -Value $voiceName -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'MilenaExeRelativePath' -Value 'milena.exe' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'MbrolaExePath' -Value $mbrolaExeTokenValue -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'MbrolaVoicePath' -Value $mbrolaVoiceTokenValue -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'BaseTempoPercent' -Value ([string]$BaseTempoPercent) -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $tokenKey -Name 'BasePitchPercent' -Value ([string]$BasePitchPercent) -PropertyType String -Force | Out-Null

    New-ItemProperty -Path $attrKey -Name 'Name' -Value 'Milena MBROLA' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Vendor' -Value 'Milena + MBROLA' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Language' -Value '0415' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Gender' -Value 'Female' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $attrKey -Name 'Age' -Value 'Adult' -PropertyType String -Force | Out-Null
}

$comRoots = if ($MachineWide) {
    @(
        @{ BaseKey = 'HKLM:\SOFTWARE\Classes\CLSID'; Dll = $x64Dll },
        @{ BaseKey = 'HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID'; Dll = $x86Dll }
    )
} else {
    @(
        @{ BaseKey = 'HKCU:\Software\Classes\CLSID'; Dll = $x64Dll },
        @{ BaseKey = 'HKCU:\Software\Classes\WOW6432Node\CLSID'; Dll = $x86Dll }
    )
}

$tokenRoots = if ($MachineWide) {
    @(
        'HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens',
        'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens'
    )
} else {
    @(
        'HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens',
        'HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens'
    )
}

foreach ($root in $tokenRoots) {
    $tokenPath = Join-Path $root $tokenName
    if (Test-Path $tokenPath) {
        Remove-Item -Recurse -Force $tokenPath
    }
}

foreach ($item in $comRoots) {
    Set-ComRegistration -BaseKey $item.BaseKey -DllPath $item.Dll -Clsid $engineClsid -DisplayName 'Milena SAPI5 Engine'
}

foreach ($root in $tokenRoots) {
    Set-VoiceToken -BaseKey $root
}

[pscustomobject]@{
    Voice = $voiceName
    Token = $tokenName
    X64Dll = $x64Dll
    X86Dll = $x86Dll
    MbrolaExePath = $MbrolaExePath
    MbrolaVoicePath = $MbrolaVoicePath
    MachineWide = [bool]$MachineWide
}
