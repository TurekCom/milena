# Milena Android

Projekt Android TTS oparty o Milenę i polski głos `mbrola_pl1`, z interfejsem ustawień inspirowanym aplikacją `WP Festival`.

## Co zawiera
- usługę `TextToSpeechService`, którą Android może wybrać jako silnik TTS,
- ekran ustawień z regulacją prędkości, wysokości, głośności, interpunkcji i słownika TXT,
- preview odsłuchu bez wychodzenia z aplikacji,
- spakowany runtime `Milena + mbrola_pl1`,
- natywny backend `milena` oraz `MBROLA` dla `arm64-v8a` i `x86_64`.

## Build
1. Doinstaluj w SDK: `platforms;android-36`, `build-tools;36.1.0`.
2. Wejdź do katalogu `android`.
3. Uruchom `..\build_milena_android.ps1`, żeby zbudować natywne binarki i potem APK/AAB.

## Artefakty
- `app/build/outputs/apk/debug/app-debug.apk`
- `app/build/outputs/apk/release/app-release.apk`
- `app/build/outputs/bundle/release/app-release.aab`
- `../dist/android/MilenaAndroid-1.0.0-debug.apk`
- `../dist/android/MilenaAndroid-1.0.0-release.apk`
- `../dist/android/MilenaAndroid-1.0.0-release.aab`

## Uwagi
- Aplikacja zawiera jeden polski głos MBROLA i osobny backend `libmbrola_exec.so`.
- Backend Mileny jest uruchamiany z `nativeLibraryDir`, a dane są kopiowane do `filesDir` przy pierwszym starcie.
