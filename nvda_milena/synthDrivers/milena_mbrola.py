# coding: utf-8
from __future__ import annotations

import addonHandler
addonHandler.initTranslation()

from array import array
import buildVersion
import os
import queue
import subprocess
import tempfile
import threading
from pathlib import Path

import config
import logHandler
import nvwave
from speech.commands import IndexCommand
from synthDriverHandler import SynthDriver as NvdaSynthDriver, VoiceInfo, synthDoneSpeaking, synthIndexReached


log = logHandler.log
CREATE_NO_WINDOW = getattr(subprocess, "CREATE_NO_WINDOW", 0)
SAMPLE_RATE = 16000
DEBUG_LOG = Path(tempfile.gettempdir()) / "milena_nvda_debug.log"


def _debug(message: str) -> None:
    try:
        with DEBUG_LOG.open("a", encoding="utf-8") as handle:
            handle.write(message + "\n")
    except Exception:
        pass


def _driver_dir() -> Path:
    return Path(__file__).resolve().parent


def _runtime_dir() -> Path | None:
    candidate = _driver_dir() / "milena_runtime"
    required = (
        candidate / "milena.exe",
        candidate / "data",
        candidate / "mbrola" / "mbrola.exe",
        candidate / "mbrola" / "pl1",
    )
    if all(path.exists() for path in required):
        _debug(f"runtime-ok {candidate}")
        return candidate
    _debug(f"runtime-missing {candidate}")
    return None


def _collapse_whitespace(text: str) -> str:
    return " ".join(text.replace("\r", " ").replace("\n", " ").replace("\t", " ").split())


def _write_utf8_text(path: Path, text: str) -> None:
    path.write_text(text, encoding="utf-8-sig")


def _clamp_percent(value: int, minimum: int = 0, maximum: int = 100) -> int:
    return max(minimum, min(maximum, int(value)))


def _tempo_percent_for_rate(rate: int) -> int:
    # MBROLA's -t works as a duration multiplier:
    # larger values produce slower speech, smaller values produce faster speech.
    # NVDA sliders are expected to behave the other way around:
    # lower percentage => slower, higher percentage => faster.
    return max(25, min(400, 150 - _clamp_percent(rate)))


def _apply_volume_to_pcm(pcm: bytes, volume: int) -> bytes:
    if not pcm:
        return b""
    level = _clamp_percent(volume)
    if level >= 100:
        return pcm
    if level <= 0:
        return b"\x00" * len(pcm)
    gain = level / 100.0
    samples = array("h")
    samples.frombytes(pcm)
    for idx, sample in enumerate(samples):
        scaled = int(round(sample * gain))
        samples[idx] = max(-32768, min(32767, scaled))
    return samples.tobytes()


def _run_with_redirect(
    command: list[str],
    cwd: Path,
    stdin_path: Path | None = None,
    stdout_path: Path | None = None,
) -> subprocess.Popen[bytes]:
    stdin_handle = open(stdin_path, "rb") if stdin_path else subprocess.DEVNULL
    stdout_handle = open(stdout_path, "wb") if stdout_path else subprocess.DEVNULL
    try:
        return subprocess.Popen(
            command,
            cwd=str(cwd),
            stdin=stdin_handle,
            stdout=stdout_handle,
            stderr=subprocess.DEVNULL,
            creationflags=CREATE_NO_WINDOW,
        )
    finally:
        if stdin_path:
            stdin_handle.close()
        if stdout_path:
            stdout_handle.close()


def synthesize_text_to_pcm(runtime_dir: Path, text: str, rate: int, pitch: int, volume: int, process_callback=None) -> bytes:
    clean_text = _collapse_whitespace(text)
    if not clean_text:
        _debug("synthesize-empty-text")
        return b""

    rate_value = _clamp_percent(rate)
    pitch_value = _clamp_percent(pitch)
    volume_value = _clamp_percent(volume)
    tempo_percent = _tempo_percent_for_rate(rate_value)
    pitch_percent = max(25, min(400, 50 + pitch_value))

    txt_fd, txt_name = tempfile.mkstemp(suffix=".txt")
    pho_fd, pho_name = tempfile.mkstemp(suffix=".pho")
    raw_fd, raw_name = tempfile.mkstemp(suffix=".raw")
    os.close(txt_fd)
    os.close(pho_fd)
    os.close(raw_fd)

    txt_path = Path(txt_name)
    pho_path = Path(pho_name)
    raw_path = Path(raw_name)

    try:
        _write_utf8_text(txt_path, clean_text)
        _debug(
            "synthesize-start "
            f"text={clean_text!r} rate={rate_value} pitch={pitch_value} volume={volume_value} "
            f"tempo={tempo_percent} pitchPct={pitch_percent}"
        )

        milena_cmd = [str(runtime_dir / "milena.exe"), "-U"]
        milena_proc = _run_with_redirect(milena_cmd, runtime_dir, stdin_path=txt_path, stdout_path=pho_path)
        if process_callback:
            process_callback(milena_proc)
        milena_proc.wait(timeout=60.0)
        if process_callback:
            process_callback(None)
        if milena_proc.returncode != 0:
            _debug(f"milena-failed rc={milena_proc.returncode}")
            raise subprocess.CalledProcessError(milena_proc.returncode, milena_cmd)

        mbrola_cmd = [
            str(runtime_dir / "mbrola" / "mbrola.exe"),
            "-e",
            "-f",
            f"{pitch_percent / 100.0:.3f}",
            "-t",
            f"{tempo_percent / 100.0:.3f}",
            str(runtime_dir / "mbrola" / "pl1"),
            str(pho_path),
            str(raw_path),
        ]
        mbrola_proc = subprocess.Popen(
            mbrola_cmd,
            cwd=str(runtime_dir),
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            creationflags=CREATE_NO_WINDOW,
        )
        if process_callback:
            process_callback(mbrola_proc)
        mbrola_proc.wait(timeout=60.0)
        if process_callback:
            process_callback(None)
        if mbrola_proc.returncode != 0:
            _debug(f"mbrola-failed rc={mbrola_proc.returncode}")
            raise subprocess.CalledProcessError(mbrola_proc.returncode, mbrola_cmd)

        data = _apply_volume_to_pcm(raw_path.read_bytes(), volume_value)
        _debug(f"synthesize-done bytes={len(data)}")
        return data
    finally:
        if process_callback:
            process_callback(None)
        for path in (txt_path, pho_path, raw_path):
            try:
                path.unlink()
            except OSError:
                pass


RUNTIME_DIR = _runtime_dir()
VOICE = VoiceInfo(id="milena_mbrola", language="pl", displayName="Milena - MBROLA")


class SynthDriver(NvdaSynthDriver):
    name = "milena_mbrola"
    description = "Milena - MBROLA"

    supportedSettings = (
        NvdaSynthDriver.RateSetting(),
        NvdaSynthDriver.PitchSetting(),
        NvdaSynthDriver.VolumeSetting(),
    )
    supportedCommands = {IndexCommand}
    supportedNotifications = {synthIndexReached, synthDoneSpeaking}

    _rate = 50
    _pitch = 50
    _volume = 100

    def __init__(self):
        super().__init__()
        if not RUNTIME_DIR:
            raise RuntimeError("Milena runtime not found")
        _debug("driver-init")
        self._rate = 50
        self._pitch = 50
        self._volume = 100
        device = (
            config.conf["audio"]["outputDevice"]
            if getattr(buildVersion, "version_year", 2025) >= 2025
            else config.conf["speech"]["outputDevice"]
        )
        self.player = nvwave.WavePlayer(
            channels=1,
            samplesPerSec=SAMPLE_RATE,
            bitsPerSample=16,
            outputDevice=device,
        )
        self._queue: queue.Queue[tuple[str, int | None, int, int, int]] = queue.Queue()
        self._stopEvent = threading.Event()
        self._stateLock = threading.Lock()
        self._currentProcess: subprocess.Popen[bytes] | None = None
        self._speaking = False
        self._cancelSerial = 0
        self._thread = threading.Thread(target=self._run, daemon=True, name="MilenaNvdaWorker")
        self._thread.start()

    @classmethod
    def check(cls):
        return RUNTIME_DIR is not None

    def getAvailableVoices(self):
        return {"milena_mbrola": VOICE}

    def getVoice(self):
        return "milena_mbrola"

    def speak(self, speechSequence):
        text_parts: list[str] = []
        pending_index: int | None = None
        for item in speechSequence:
            if isinstance(item, str):
                text_parts.append(item)
            elif isinstance(item, IndexCommand):
                pending_index = item.index
        text = _collapse_whitespace("".join(text_parts))
        if not text and pending_index is None:
            return
        _debug(f"speak-queue text={text!r} index={pending_index}")
        self._stopEvent.clear()
        self._queue.put((text, pending_index, int(self._rate), int(self._pitch), int(self._volume)))

    def cancel(self):
        _debug("cancel")
        with self._stateLock:
            self._cancelSerial += 1
            process = self._currentProcess
            self._currentProcess = None
        while not self._queue.empty():
            try:
                self._queue.get_nowait()
                self._queue.task_done()
            except queue.Empty:
                break
        if process and process.poll() is None:
            try:
                process.terminate()
                process.wait(timeout=1.0)
            except Exception:
                try:
                    process.kill()
                except Exception:
                    pass
        self.player.stop()
        self._speaking = False

    def isSpeaking(self):
        return self._speaking

    def pause(self, switch):
        self.player.pause(switch)

    def terminate(self):
        self._stopEvent.set()
        self.cancel()
        self._queue.put(("", None, 50, 50, 100))
        if self._thread.is_alive():
            self._thread.join(timeout=1.0)
        self.player.close()

    def _get_cancel_serial(self) -> int:
        with self._stateLock:
            return self._cancelSerial

    def _was_cancelled(self, serial: int) -> bool:
        return serial != self._get_cancel_serial()

    def _run(self):
        while not self._stopEvent.is_set():
            try:
                text, index, rate, pitch, volume = self._queue.get(timeout=0.1)
            except queue.Empty:
                continue
            try:
                if self._stopEvent.is_set():
                    continue
                if not text and index is None:
                    continue
                serial = self._get_cancel_serial()
                _debug(f"worker-start text={text!r} index={index} serial={serial}")
                if text:
                    self._speaking = True
                    pcm = synthesize_text_to_pcm(RUNTIME_DIR, text, rate, pitch, volume, self._set_current_process)
                    if self._was_cancelled(serial):
                        _debug("worker-cancelled-after-synth")
                        continue
                    if pcm:
                        _debug(f"player-feed bytes={len(pcm)}")
                        self.player.feed(pcm)
                        self.player.idle()
                if self._was_cancelled(serial):
                    _debug("worker-cancelled-before-notify")
                    continue
                if index is not None:
                    self.lastIndex = index
                    synthIndexReached.notify(synth=self, index=index)
                synthDoneSpeaking.notify(synth=self)
                _debug("worker-done")
            except Exception:
                _debug("worker-exception")
                log.error("Milena NVDA error", exc_info=True)
            finally:
                self._speaking = False
                self._queue.task_done()

    def _set_current_process(self, process: subprocess.Popen[bytes] | None):
        with self._stateLock:
            self._currentProcess = process

    def _get_rate(self):
        return self._rate

    def _set_rate(self, value):
        self._rate = _clamp_percent(value)

    def _get_pitch(self):
        return self._pitch

    def _set_pitch(self, value):
        self._pitch = _clamp_percent(value)

    def _get_volume(self):
        return self._volume

    def _set_volume(self, value):
        level = _clamp_percent(value)
        if level == 0:
            # Existing NVDA config may persist volume=0 for this driver,
            # which makes the synth appear broken from the first launch.
            _debug("volume-zero-fallback-to-100")
            level = 100
        self._volume = level
