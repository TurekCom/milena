# Releasing

## Local Android signing

The Android project reads local signing credentials from:

- `android/release-signing.properties`
- `android/*.jks`
- `android/*.keystore`

These files are ignored by Git. Start from `android/release-signing.properties.example`.

## GitHub Actions secrets

The release workflow expects these optional repository secrets for Android signing:

- `ANDROID_KEYSTORE_BASE64`
- `ANDROID_KEYSTORE_PASSWORD`
- `ANDROID_KEY_ALIAS`
- `ANDROID_KEY_PASSWORD`

If the secrets are present, the workflow creates `android/release-signing.properties` on the runner and signs the Android release with that keystore.

## Local release build

Run:

```powershell
.\build_milena_installer.ps1
python .\build_milena_nvda_addon.py
.\build_milena_android.ps1
.\build_release_checksums.ps1
```

## GitHub release update

Run:

```powershell
gh release upload v1.0.0 .\dist\release\Milena-1.0.0-SHA256SUMS.txt --clobber
```
