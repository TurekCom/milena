# Milena

Milena is a Polish text-to-speech frontend that translates text into an intermediate phonetic form and can drive MBROLA-based speech output.

This repository contains:
- the original Milena engine sources and data,
- a Windows SAPI5 voice package,
- an NVDA synth driver/add-on,
- an Android `TextToSpeechService` package based on `Milena -> MBROLA -> PCM`.

## Components

### Windows SAPI5
- source: `sapi5_milena/`
- build script: `build_milena_sapi5.ps1`
- installer build: `build_milena_installer.ps1`

### NVDA add-on
- source: `nvda_milena/`
- build script: `build_milena_nvda_addon.py`

### Android
- source: `android/`
- build script: `build_milena_android.ps1`
- output artifacts:
  - `dist/android/MilenaAndroid-1.0.0-debug.apk`
  - `dist/android/MilenaAndroid-1.0.0-release.apk`
  - `dist/android/MilenaAndroid-1.0.0-release.aab`

## Build

### Windows engine smoke build
Run:

```powershell
.\build_windows.ps1
```

### Windows SAPI5
Run:

```powershell
.\build_milena_sapi5.ps1
.\build_milena_installer.ps1
```

### NVDA add-on
Run:

```powershell
python .\build_milena_nvda_addon.py
```

### Android
Requirements:
- Android SDK with API 36 / build-tools 36.x
- Python available in `PATH`
- Windows host

Run:

```powershell
.\build_milena_android.ps1
```

The Android build uses NDK r17c because newer Clang-based NDKs do not compile Milena cleanly due to GNU nested functions used in the original C sources.

## Release Artifacts

Recommended release assets for GitHub:
- `dist/installer/MilenaMBROLA-SAPI5-1.0.0-x64.exe`
- `dist/nvda/Milena_MBROLA-1.0.0.nvda-addon`
- `dist/android/MilenaAndroid-1.0.0-release.apk`
- `dist/android/MilenaAndroid-1.0.0-release.aab`

## Notes

- The legacy upstream documentation is kept in the original files such as `README`, `README_phraser`, `README_udict`, `README_utils`.
- Licensing notes from the original project are preserved in `LICENCJA`.
