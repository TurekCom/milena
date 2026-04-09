import os
import shutil
import subprocess
import zipfile
from pathlib import Path


ADDON_VERSION = "1.0.0"
ROOT = Path(__file__).resolve().parent
BUILD_DIR = ROOT / "build_addon_milena"
DIST_DIR = ROOT / "dist" / "nvda"
SOURCE_DIR = ROOT / "nvda_milena"
RUNTIME_DIR = ROOT / "sapi5_milena" / "dist"
ADDON_FILE = DIST_DIR / f"Milena_MBROLA-{ADDON_VERSION}.nvda-addon"


def run_build_runtime():
    subprocess.check_call([
        "powershell",
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        str(ROOT / "build_milena_sapi5.ps1"),
    ])


def prepare_structure():
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
    (BUILD_DIR / "synthDrivers" / "milena_runtime").mkdir(parents=True, exist_ok=True)
    shutil.copy2(SOURCE_DIR / "manifest.ini", BUILD_DIR / "manifest.ini")
    shutil.copy2(SOURCE_DIR / "synthDrivers" / "milena_mbrola.py", BUILD_DIR / "synthDrivers" / "milena_mbrola.py")


def copy_runtime():
    if not (RUNTIME_DIR / "milena.exe").exists():
        raise FileNotFoundError(f"Brak milena.exe w stagingu: {RUNTIME_DIR}")
    if not (RUNTIME_DIR / "data").exists():
        raise FileNotFoundError(f"Brak katalogu data w stagingu: {RUNTIME_DIR}")
    if not (RUNTIME_DIR / "mbrola" / "mbrola.exe").exists():
        raise FileNotFoundError(f"Brak mbrola.exe w stagingu: {RUNTIME_DIR}")

    runtime_dst = BUILD_DIR / "synthDrivers" / "milena_runtime"
    shutil.copy2(RUNTIME_DIR / "milena.exe", runtime_dst / "milena.exe")
    shutil.copytree(RUNTIME_DIR / "data", runtime_dst / "data", dirs_exist_ok=True)
    shutil.copytree(RUNTIME_DIR / "mbrola", runtime_dst / "mbrola", dirs_exist_ok=True)


def create_addon_file():
    DIST_DIR.mkdir(parents=True, exist_ok=True)
    if ADDON_FILE.exists():
        ADDON_FILE.unlink()
    with zipfile.ZipFile(ADDON_FILE, "w", zipfile.ZIP_DEFLATED) as zipf:
        for root, _, files in os.walk(BUILD_DIR):
            for file_name in files:
                file_path = Path(root) / file_name
                zipf.write(file_path, file_path.relative_to(BUILD_DIR))


if __name__ == "__main__":
    run_build_runtime()
    prepare_structure()
    copy_runtime()
    create_addon_file()
    print(ADDON_FILE)
