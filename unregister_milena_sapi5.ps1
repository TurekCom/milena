param(
    [switch]$MachineWide
)

$ErrorActionPreference = 'Stop'

$engineClsid = '{0E23AAFA-4F1D-4A09-A8A4-759A84EAC219}'
$tokenName = 'MilenaMbrola.Standard'

$paths = if ($MachineWide) {
    @(
        "HKLM:\SOFTWARE\Classes\CLSID\$engineClsid",
        "HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\$engineClsid",
        "HKLM:\SOFTWARE\Microsoft\Speech\Voices\Tokens\$tokenName",
        "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens\$tokenName"
    )
} else {
    @(
        "HKCU:\Software\Classes\CLSID\$engineClsid",
        "HKCU:\Software\Classes\WOW6432Node\CLSID\$engineClsid",
        "HKCU:\SOFTWARE\Microsoft\Speech\Voices\Tokens\$tokenName",
        "HKCU:\SOFTWARE\WOW6432Node\Microsoft\Speech\Voices\Tokens\$tokenName"
    )
}

foreach ($path in $paths) {
    if (Test-Path $path) {
        Remove-Item -Recurse -Force $path
    }
}

[pscustomobject]@{
    Removed = $paths -join '; '
}
