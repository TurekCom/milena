# Milena v1.0.0

First packaged public release of the Milena ports and integrations prepared in this repository.

## Included

- Windows SAPI5 voice package for Milena + MBROLA
- NVDA add-on for Milena + MBROLA
- Android TTS engine package (`TextToSpeechService`)

## Artifacts

- `MilenaMBROLA-SAPI5-1.0.0-x64.exe`
- `Milena_MBROLA-1.0.0.nvda-addon`
- `MilenaAndroid-1.0.0-release.apk`
- `MilenaAndroid-1.0.0-release.aab`

## Verification

- Windows SAPI5 tested on x64/x86 hosts
- NVDA add-on verified locally
- Android package installed and exercised on Pixel 9 Pro through ADB
- Android runtime confirmed on-device:
  - service registered as `android.intent.action.TTS_SERVICE`
  - runtime initialized for `arm64`
  - synthesis reached `AudioTrack`

## Notes

- Android builds use NDK r17c because the original Milena C sources rely on GNU nested functions that are not accepted by newer Clang-based NDK toolchains.
- The repository keeps the original upstream documentation alongside the new Windows and Android integration code.
