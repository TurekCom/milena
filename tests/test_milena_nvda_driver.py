import importlib.util
import os
import sys
import types
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DRIVER_PATH = ROOT / "nvda_milena" / "synthDrivers" / "milena_mbrola.py"


class _NotifyAction:
    def __init__(self):
        self.calls = []

    def notify(self, **kwargs):
        self.calls.append(kwargs)


class _FakeBaseSynthDriver:
    def __init__(self):
        self.lastIndex = None

    @classmethod
    def RateSetting(cls):
        return ("rate",)

    @classmethod
    def PitchSetting(cls):
        return ("pitch",)

    @classmethod
    def VolumeSetting(cls):
        return ("volume",)

    @property
    def rate(self):
        return self._get_rate()

    @rate.setter
    def rate(self, value):
        self._set_rate(value)

    @property
    def pitch(self):
        return self._get_pitch()

    @pitch.setter
    def pitch(self, value):
        self._set_pitch(value)

    @property
    def volume(self):
        return self._get_volume()

    @volume.setter
    def volume(self, value):
        self._set_volume(value)


class _FakeWavePlayer:
    def __init__(self, **kwargs):
        self.kwargs = kwargs
        self.feeds = []
        self.stopped = 0
        self.closed = 0
        self.paused = []

    def feed(self, data):
        self.feeds.append(bytes(data))

    def idle(self):
        return

    def stop(self):
        self.stopped += 1

    def close(self):
        self.closed += 1

    def pause(self, switch):
        self.paused.append(bool(switch))


class _FakeIndexCommand:
    def __init__(self, index):
        self.index = index


def _load_driver_module():
    for key in list(sys.modules):
        if key in {
            "milena_nvda_driver_under_test",
            "addonHandler",
            "buildVersion",
            "config",
            "logHandler",
            "nvwave",
            "speech",
            "speech.commands",
            "synthDriverHandler",
        }:
            del sys.modules[key]

    addon_handler = types.ModuleType("addonHandler")
    addon_handler.initTranslation = lambda: None
    sys.modules["addonHandler"] = addon_handler

    build_version = types.ModuleType("buildVersion")
    build_version.version_year = 2025
    sys.modules["buildVersion"] = build_version

    config = types.ModuleType("config")
    config.conf = {
        "audio": {"outputDevice": None},
        "speech": {"outputDevice": None},
    }
    sys.modules["config"] = config

    log_handler = types.ModuleType("logHandler")
    log_handler.log = types.SimpleNamespace(error=lambda *a, **k: None)
    sys.modules["logHandler"] = log_handler

    nvwave = types.ModuleType("nvwave")
    nvwave.WavePlayer = _FakeWavePlayer
    sys.modules["nvwave"] = nvwave

    speech_pkg = types.ModuleType("speech")
    speech_commands = types.ModuleType("speech.commands")
    speech_commands.IndexCommand = _FakeIndexCommand
    sys.modules["speech"] = speech_pkg
    sys.modules["speech.commands"] = speech_commands

    synth_driver_handler = types.ModuleType("synthDriverHandler")
    synth_driver_handler.SynthDriver = _FakeBaseSynthDriver
    synth_driver_handler.VoiceInfo = lambda **kwargs: types.SimpleNamespace(**kwargs)
    synth_driver_handler.synthDoneSpeaking = _NotifyAction()
    synth_driver_handler.synthIndexReached = _NotifyAction()
    sys.modules["synthDriverHandler"] = synth_driver_handler

    spec = importlib.util.spec_from_file_location("milena_nvda_driver_under_test", DRIVER_PATH)
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    module.RUNTIME_DIR = Path("C:/fake/milena_runtime")
    return module


class MilenaNvdaDriverTests(unittest.TestCase):
    def test_driver_speaks_and_notifies(self):
        module = _load_driver_module()
        module.synthesize_text_to_pcm = lambda runtime_dir, text, rate, pitch, volume, process_callback=None: b"pcm:" + text.encode("utf-8")
        synth = module.SynthDriver()
        try:
            synth.speak(["Ala", _FakeIndexCommand(4), " ma kota"])
            synth._queue.join()
            self.assertEqual(len(synth.player.feeds), 1)
            self.assertEqual(synth.player.feeds[0], b"pcm:Ala ma kota")
            self.assertEqual(synth.lastIndex, 4)
            self.assertEqual(len(module.synthIndexReached.calls), 1)
            self.assertEqual(len(module.synthDoneSpeaking.calls), 1)
        finally:
            synth.terminate()

    def test_cancel_stops_player(self):
        module = _load_driver_module()
        module.synthesize_text_to_pcm = lambda runtime_dir, text, rate, pitch, volume, process_callback=None: b"pcm"
        synth = module.SynthDriver()
        try:
            synth.cancel()
            self.assertEqual(synth.player.stopped, 1)
        finally:
            synth.terminate()

    def test_apply_volume_to_pcm_handles_zero_and_scaling(self):
        module = _load_driver_module()
        pcm = (1000).to_bytes(2, "little", signed=True) + (-1000).to_bytes(2, "little", signed=True)
        self.assertEqual(module._apply_volume_to_pcm(pcm, 0), b"\x00\x00\x00\x00")
        half = module._apply_volume_to_pcm(pcm, 50)
        self.assertEqual(int.from_bytes(half[:2], "little", signed=True), 500)
        self.assertEqual(int.from_bytes(half[2:], "little", signed=True), -500)
        self.assertEqual(module._apply_volume_to_pcm(pcm, 100), pcm)

    def test_zero_volume_setting_falls_back_to_full_volume(self):
        module = _load_driver_module()
        synth = module.SynthDriver()
        try:
            synth._set_volume(0)
            self.assertEqual(synth._get_volume(), 100)
        finally:
            synth.terminate()

    def test_tempo_percent_for_rate_matches_slider_direction(self):
        module = _load_driver_module()
        self.assertGreater(module._tempo_percent_for_rate(0), module._tempo_percent_for_rate(50))
        self.assertGreater(module._tempo_percent_for_rate(50), module._tempo_percent_for_rate(100))
        self.assertEqual(module._tempo_percent_for_rate(50), 100)


if __name__ == "__main__":
    unittest.main()
