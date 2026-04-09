package com.turek.milena.android

import android.speech.tts.TextToSpeech
import android.speech.tts.Voice
import java.util.Locale

enum class MilenaVoiceFamily(val id: String, val displayName: String) {
    STANDARD("standard", "Standard"),
}

data class MilenaPreset(
    val id: String,
    val displayName: String,
    val baseTempoPercent: Int,
    val basePitchPercent: Int,
    val baseVolumePercent: Int,
)

data class MilenaVoiceSpec(
    val name: String,
    val family: MilenaVoiceFamily,
    val preset: MilenaPreset,
) {
    val displayName: String = "Milena MBROLA - ${preset.displayName}"
}

object MilenaEngine {
    val LOCALE: Locale = Locale.Builder().setLanguage("pl").setRegion("PL").build()
    private const val CHECK_VOICE_DATA_ENTRY = "pol-POL"
    const val RUNTIME_ROOT_NAME = "milena_runtime"
    const val RUNTIME_ASSET_PATH = "runtime/common/milena_root"
    const val MILENA_EXECUTABLE_NAME = "libmilena_exec.so"
    const val MBROLA_EXECUTABLE_NAME = "libmbrola_exec.so"
    const val SAMPLE_RATE = 16000

    private val languageCodes = setOf("pl", "pol")
    private val countryCodes = setOf("", "pl", "pol")

    private val standardPreset = MilenaPreset(
        id = "standard",
        displayName = "Standard",
        baseTempoPercent = 100,
        basePitchPercent = 100,
        baseVolumePercent = 100,
    )

    val voices: List<MilenaVoiceSpec> = listOf(
        MilenaVoiceSpec(
            name = "milena_mbrola_pl_standard",
            family = MilenaVoiceFamily.STANDARD,
            preset = standardPreset,
        ),
    )

    fun defaultVoice(): MilenaVoiceSpec = voices.first()

    fun checkVoiceDataEntries(): List<String> = listOf(CHECK_VOICE_DATA_ENTRY)

    fun voice(name: String?): MilenaVoiceSpec? = voices.firstOrNull { it.name == name }

    fun supportedFamilies(): List<MilenaVoiceFamily> = voices.map { it.family }.distinct()

    fun presetsForFamily(family: MilenaVoiceFamily): List<MilenaPreset> =
        if (family == MilenaVoiceFamily.STANDARD) listOf(standardPreset) else emptyList()

    fun voiceFor(family: MilenaVoiceFamily, presetId: String): MilenaVoiceSpec =
        voices.firstOrNull { it.family == family && it.preset.id == presetId } ?: defaultVoice()

    fun toAndroidVoice(spec: MilenaVoiceSpec): Voice =
        Voice(
            spec.name,
            LOCALE,
            Voice.QUALITY_NORMAL,
            Voice.LATENCY_NORMAL,
            false,
            setOf(TextToSpeech.Engine.KEY_FEATURE_EMBEDDED_SYNTHESIS),
        )

    fun iso3Language(): String = try {
        LOCALE.isO3Language
    } catch (_: Exception) {
        "pol"
    }

    fun iso3Country(): String = try {
        LOCALE.isO3Country
    } catch (_: Exception) {
        "POL"
    }

    fun matches(language: String?, country: String?): Boolean {
        val lang = (language ?: "").lowercase(Locale.ROOT)
        val ctr = (country ?: "").lowercase(Locale.ROOT)
        return lang in languageCodes && ctr in countryCodes
    }
}
