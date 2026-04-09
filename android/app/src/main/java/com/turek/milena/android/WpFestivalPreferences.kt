package com.turek.milena.android

import android.content.Context

object MilenaRanges {
    const val RATE_MIN = 0
    const val RATE_MAX = 200
    const val RATE_NEUTRAL = 100
    const val PITCH_MIN = 0
    const val PITCH_MAX = 200
    const val PITCH_NEUTRAL = 100
    const val VOLUME_MIN = 0
    const val VOLUME_MAX = 200
    const val VOLUME_NEUTRAL = 100
}

enum class PunctuationVerbosity(val storageValue: String) {
    ALL("all"),
    SOME("some"),
    MOST("most"),
    NONE("none");

    companion object {
        fun fromStorage(value: String?): PunctuationVerbosity =
            entries.firstOrNull { it.storageValue == value } ?: NONE
    }
}

data class EngineSettings(
    val voiceName: String = MilenaEngine.defaultVoice().name,
    val ratePercent: Int = MilenaRanges.RATE_NEUTRAL,
    val pitchPercent: Int = MilenaRanges.PITCH_NEUTRAL,
    val volumePercent: Int = MilenaRanges.VOLUME_NEUTRAL,
    val speakEmoji: Boolean = true,
    val punctuationVerbosity: PunctuationVerbosity = PunctuationVerbosity.NONE,
    val dictionaryName: String? = null,
)

object MilenaPreferences {
    private const val PREFS_NAME = "milena_android_settings"
    private const val SETTINGS_VERSION = 1
    private const val KEY_SETTINGS_VERSION = "settings_version"
    private const val KEY_VOICE_NAME = "voice_name"
    private const val KEY_RATE = "rate_percent"
    private const val KEY_PITCH = "pitch_percent"
    private const val KEY_VOLUME = "volume_percent"
    private const val KEY_SPEAK_EMOJI = "speak_emoji"
    private const val KEY_PUNCTUATION_VERBOSITY = "punctuation_verbosity"
    private const val KEY_DICTIONARY_NAME = "dictionary_name"

    fun load(context: Context): EngineSettings {
        val prefs = context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
        prefs.getInt(KEY_SETTINGS_VERSION, 0)
        return EngineSettings(
            voiceName = prefs.getString(KEY_VOICE_NAME, MilenaEngine.defaultVoice().name)
                ?: MilenaEngine.defaultVoice().name,
            ratePercent = prefs.getInt(KEY_RATE, MilenaRanges.RATE_NEUTRAL)
                .coerceIn(MilenaRanges.RATE_MIN, MilenaRanges.RATE_MAX),
            pitchPercent = prefs.getInt(KEY_PITCH, MilenaRanges.PITCH_NEUTRAL)
                .coerceIn(MilenaRanges.PITCH_MIN, MilenaRanges.PITCH_MAX),
            volumePercent = prefs.getInt(KEY_VOLUME, MilenaRanges.VOLUME_NEUTRAL)
                .coerceIn(MilenaRanges.VOLUME_MIN, MilenaRanges.VOLUME_MAX),
            speakEmoji = prefs.getBoolean(KEY_SPEAK_EMOJI, true),
            punctuationVerbosity = PunctuationVerbosity.fromStorage(
                prefs.getString(KEY_PUNCTUATION_VERBOSITY, PunctuationVerbosity.NONE.storageValue),
            ),
            dictionaryName = prefs.getString(KEY_DICTIONARY_NAME, null),
        )
    }

    fun save(context: Context, settings: EngineSettings) {
        context.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)
            .edit()
            .putInt(KEY_SETTINGS_VERSION, SETTINGS_VERSION)
            .putString(KEY_VOICE_NAME, settings.voiceName)
            .putInt(KEY_RATE, settings.ratePercent.coerceIn(MilenaRanges.RATE_MIN, MilenaRanges.RATE_MAX))
            .putInt(KEY_PITCH, settings.pitchPercent.coerceIn(MilenaRanges.PITCH_MIN, MilenaRanges.PITCH_MAX))
            .putInt(KEY_VOLUME, settings.volumePercent.coerceIn(MilenaRanges.VOLUME_MIN, MilenaRanges.VOLUME_MAX))
            .putBoolean(KEY_SPEAK_EMOJI, settings.speakEmoji)
            .putString(KEY_PUNCTUATION_VERBOSITY, settings.punctuationVerbosity.storageValue)
            .putString(KEY_DICTIONARY_NAME, settings.dictionaryName)
            .apply()
    }
}
