param(
    [string]$VoiceNameLike = 'Milena',
    [string]$OutFile = (Join-Path $PWD 'milena_sapi_test.wav'),
    [string]$Text = 'Zażółć gęślą jaźń.'
)

$ErrorActionPreference = 'Stop'

$voice = New-Object -ComObject SAPI.SpVoice
$voices = @($voice.GetVoices())
$match = $voices | Where-Object { $_.GetDescription() -like "*$VoiceNameLike*" } | Select-Object -First 1
if (-not $match) {
    throw "Nie znaleziono głosu SAPI pasującego do: $VoiceNameLike"
}

$stream = New-Object -ComObject SAPI.SpFileStream
$stream.Open($OutFile, 3, $false)
$voice.Voice = $match
$voice.AudioOutputStream = $stream
[void]$voice.Speak($Text)
$stream.Close()

[pscustomobject]@{
    Voice = $match.GetDescription()
    OutFile = $OutFile
    Length = (Get-Item $OutFile).Length
}
