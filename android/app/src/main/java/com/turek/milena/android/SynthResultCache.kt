package com.turek.milena.android

object SynthResultCache {
    private const val MAX_ENTRIES = 48
    private const val MAX_TOTAL_PCM_BYTES = 3 * 1024 * 1024
    private const val MAX_TEXT_LENGTH = 180
    private const val MAX_PCM_BYTES_PER_ENTRY = 256 * 1024

    private data class CacheEntry(
        val result: SynthResult,
        val byteSize: Int,
    )

    data class CacheKey(
        val text: String,
        val voiceName: String,
        val ratePercent: Int,
        val pitchPercent: Int,
        val volumePercent: Int,
    )

    private val cache = object : LinkedHashMap<CacheKey, CacheEntry>(MAX_ENTRIES, 0.75f, true) {
        override fun removeEldestEntry(eldest: MutableMap.MutableEntry<CacheKey, CacheEntry>?): Boolean {
            return size > MAX_ENTRIES
        }
    }

    private var totalPcmBytes = 0

    fun keyOrNull(
        text: String,
        voiceName: String,
        ratePercent: Int,
        pitchPercent: Int,
        volumePercent: Int,
    ): CacheKey? {
        val normalized = text.trim()
        if (normalized.isEmpty() || normalized.length > MAX_TEXT_LENGTH) {
            return null
        }
        return CacheKey(normalized, voiceName, ratePercent, pitchPercent, volumePercent)
    }

    @Synchronized
    fun get(key: CacheKey): SynthResult? = cache[key]?.result

    @Synchronized
    fun put(key: CacheKey, result: SynthResult) {
        if (result.pcm.isEmpty() || result.pcm.size > MAX_PCM_BYTES_PER_ENTRY) {
            return
        }
        cache.remove(key)?.let { totalPcmBytes -= it.byteSize }
        val entry = CacheEntry(result, result.pcm.size)
        cache[key] = entry
        totalPcmBytes += entry.byteSize
        trimToBudget()
    }

    @Synchronized
    fun clear() {
        cache.clear()
        totalPcmBytes = 0
    }

    private fun trimToBudget() {
        val iterator = cache.entries.iterator()
        while ((cache.size > MAX_ENTRIES || totalPcmBytes > MAX_TOTAL_PCM_BYTES) && iterator.hasNext()) {
            val entry = iterator.next()
            totalPcmBytes -= entry.value.byteSize
            iterator.remove()
        }
    }
}
