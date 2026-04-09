package com.turek.milena.android

import android.media.AudioFormat
import android.os.Bundle
import android.speech.tts.SynthesisCallback
import android.speech.tts.SynthesisRequest
import android.speech.tts.TextToSpeech
import android.speech.tts.TextToSpeechService
import android.speech.tts.Voice
import android.util.Log
import kotlin.math.roundToInt

class MilenaTtsService : TextToSpeechService() {
    companion object {
        private const val TAG = "MilenaTts"
    }

    private class SynthesisStoppedException : RuntimeException()

    @Volatile
    private var stopRequested = false

    override fun onGetLanguage(): Array<String> = arrayOf(
        MilenaEngine.iso3Language(),
        MilenaEngine.iso3Country(),
        "",
    )

    override fun onIsLanguageAvailable(language: String, country: String, variant: String): Int {
        return if (MilenaEngine.matches(language, country)) {
            TextToSpeech.LANG_COUNTRY_AVAILABLE
        } else {
            TextToSpeech.LANG_NOT_SUPPORTED
        }
    }

    override fun onLoadLanguage(language: String, country: String, variant: String): Int =
        onIsLanguageAvailable(language, country, variant)

    override fun onIsValidVoiceName(voiceName: String): Int {
        return if (MilenaEngine.voice(voiceName) != null) {
            TextToSpeech.SUCCESS
        } else {
            TextToSpeech.ERROR
        }
    }

    override fun onLoadVoice(voiceName: String): Int = onIsValidVoiceName(voiceName)

    override fun onGetDefaultVoiceNameFor(language: String, country: String, variant: String): String? {
        return if (MilenaEngine.matches(language, country)) {
            MilenaPreferences.load(this).voiceName
        } else {
            null
        }
    }

    override fun onGetVoices(): MutableList<Voice> =
        MilenaEngine.voices.map { MilenaEngine.toAndroidVoice(it) }.toMutableList()

    override fun onStop() {
        stopRequested = true
        Log.i(TAG, "Stop requested")
        MilenaRuntimeManager.stopActiveSynthesis()
    }

    override fun onSynthesizeText(request: SynthesisRequest, callback: SynthesisCallback) {
        stopRequested = false
        val originalText = request.charSequenceText?.toString() ?: request.text.orEmpty()
        val settings = MilenaPreferences.load(this)
        val dictionaryText = DictionaryRepository.apply(this, originalText)
        val normalizedText = SpeechTextNormalizer.normalize(
            this,
            dictionaryText,
            settings.speakEmoji,
            settings.punctuationVerbosity,
        )
        val sanitizedText = sanitizeForMilena(normalizedText)
            .replace(Regex("\\s+"), " ")
            .trim()
        val text = SpeechTextNormalizer.ensureSpeakableText(originalText, sanitizedText)
        if (text.isEmpty()) {
            callback.done()
            return
        }

        val segments = MilenaTextChunker.chunk(text)
        val voice = MilenaEngine.voice(request.voiceName)
            ?: MilenaEngine.voice(settings.voiceName)
            ?: MilenaEngine.defaultVoice()
        val ratePercent = mergePercent(settings.ratePercent, request.speechRate, MilenaRanges.RATE_MIN, MilenaRanges.RATE_MAX)
        val pitchPercent = mergePercent(settings.pitchPercent, request.pitch, MilenaRanges.PITCH_MIN, MilenaRanges.PITCH_MAX)
        val volumePercent = mergeVolume(settings.volumePercent, request.params)

        Log.i(
            TAG,
            "Synth start voice=${voice.name} rate=$ratePercent pitch=$pitchPercent volume=$volumePercent textLen=${text.length} segments=${segments.size}",
        )

        try {
            streamSegmentsToCallback(segments, voice, ratePercent, pitchPercent, volumePercent, settings, callback)
        } catch (_: SynthesisStoppedException) {
            if (!callback.hasFinished()) {
                callback.error(TextToSpeech.STOPPED)
            }
            return
        } catch (t: Throwable) {
            Log.e(TAG, "Synthesis failed", t)
            callback.error(if (stopRequested) TextToSpeech.STOPPED else TextToSpeech.ERROR_SYNTHESIS)
            return
        }

        if (stopRequested) {
            callback.error(TextToSpeech.STOPPED)
            return
        }
        callback.done()
    }

    private fun streamSegmentsToCallback(
        segments: List<String>,
        voice: MilenaVoiceSpec,
        ratePercent: Int,
        pitchPercent: Int,
        volumePercent: Int,
        settings: EngineSettings,
        callback: SynthesisCallback,
    ) {
        require(segments.isNotEmpty()) { "No text segments to synthesize" }

        var sampleRate: Int? = null
        for (segment in segments) {
            if (stopRequested || callback.hasFinished()) {
                throw SynthesisStoppedException()
            }
            val segmentResult = synthesizeSegmentWithFallback(
                segment = segment,
                voice = voice,
                ratePercent = ratePercent,
                pitchPercent = pitchPercent,
                volumePercent = volumePercent,
                settings = settings,
            ) ?: continue

            if (sampleRate == null) {
                val startResult = callback.start(segmentResult.sampleRate, AudioFormat.ENCODING_PCM_16BIT, 1)
                if (startResult != TextToSpeech.SUCCESS) {
                    if (stopRequested || callback.hasFinished()) {
                        throw SynthesisStoppedException()
                    }
                    throw IllegalStateException("TTS callback start failed: $startResult")
                }
                sampleRate = segmentResult.sampleRate
            } else if (sampleRate != segmentResult.sampleRate) {
                throw IllegalStateException("Sample rate changed between segments")
            }
            streamAudioChunk(segmentResult.pcm, callback)
        }

        require(sampleRate != null) { "No segment could be synthesized" }
    }

    private fun synthesizeSegmentWithFallback(
        segment: String,
        voice: MilenaVoiceSpec,
        ratePercent: Int,
        pitchPercent: Int,
        volumePercent: Int,
        settings: EngineSettings,
    ): SynthResult? {
        val cacheKey = SynthResultCache.keyOrNull(segment, voice.name, ratePercent, pitchPercent, volumePercent)
        cacheKey?.let { key ->
            SynthResultCache.get(key)?.let { return it }
        }

        return try {
            MilenaRuntimeManager.synthesize(this, segment, voice, ratePercent, pitchPercent, volumePercent)
                .also { result -> cacheKey?.let { SynthResultCache.put(it, result) } }
        } catch (primary: Throwable) {
            if (stopRequested) {
                throw SynthesisStoppedException()
            }
            val fallback = SpeechTextNormalizer.ensureSpeakableText(
                segment,
                sanitizeForMilena(
                    SpeechTextNormalizer.makeFestivalFriendly(segment, settings.punctuationVerbosity),
                ).replace(Regex("\\s+"), " ").trim(),
            )
                .replace(Regex("\\s+"), " ")
                .trim()
            if (fallback.isBlank() || fallback == segment) {
                Log.w(TAG, "Segment fallback unavailable", primary)
                null
            } else {
                Log.w(TAG, "Retrying segment with fallback: $fallback", primary)
                runCatching {
                    MilenaRuntimeManager.synthesize(this, fallback, voice, ratePercent, pitchPercent, volumePercent)
                        .also { result -> cacheKey?.let { SynthResultCache.put(it, result) } }
                }.getOrElse { fallbackError ->
                    Log.w(TAG, "Fallback synthesis failed", fallbackError)
                    null
                }
            }
        }
    }

    private fun streamAudioChunk(pcm: ByteArray, callback: SynthesisCallback) {
        if (pcm.isEmpty()) {
            return
        }
        val chunkSize = callback.maxBufferSize.coerceAtLeast(2048)
        var offset = 0
        while (offset < pcm.size) {
            if (stopRequested || callback.hasFinished()) {
                throw SynthesisStoppedException()
            }
            val size = minOf(chunkSize, pcm.size - offset)
            val status = callback.audioAvailable(pcm, offset, size)
            if (status != TextToSpeech.SUCCESS) {
                if (stopRequested || callback.hasFinished()) {
                    throw SynthesisStoppedException()
                }
                throw IllegalStateException("TTS callback audioAvailable failed: $status")
            }
            offset += size
        }
    }

    private fun mergePercent(basePercent: Int, requestPercent: Int, minPercent: Int, maxPercent: Int): Int {
        val normalizedRequest = if (requestPercent <= 0) 100 else requestPercent
        return ((basePercent.coerceIn(minPercent, maxPercent) / 100f) * normalizedRequest)
            .roundToInt()
            .coerceIn(minPercent, maxPercent)
    }

    private fun mergeVolume(basePercent: Int, params: Bundle?): Int {
        val rawVolume = params?.getFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, 1.0f) ?: 1.0f
        return (basePercent * rawVolume).roundToInt()
            .coerceIn(MilenaRanges.VOLUME_MIN, MilenaRanges.VOLUME_MAX)
    }

    private fun sanitizeForMilena(text: String): String {
        if (text.isEmpty()) {
            return text
        }
        val out = StringBuilder(text.length)
        var index = 0
        while (index < text.length) {
            val codePoint = text.codePointAt(index)
            when {
                codePoint == '\n'.code || codePoint == '\r'.code || codePoint == '\t'.code -> out.append(' ')
                isMilenaUnsafeCodePoint(codePoint) -> out.append(' ')
                else -> out.appendCodePoint(codePoint)
            }
            index += Character.charCount(codePoint)
        }
        return out.toString()
    }

    private fun isMilenaUnsafeCodePoint(codePoint: Int): Boolean {
        return when (Character.getType(codePoint)) {
            Character.CONTROL.toInt() -> codePoint != '\n'.code && codePoint != '\r'.code && codePoint != '\t'.code
            Character.FORMAT.toInt(),
            Character.SURROGATE.toInt(),
            Character.PRIVATE_USE.toInt(),
            Character.UNASSIGNED.toInt() -> true
            else -> false
        }
    }
}
