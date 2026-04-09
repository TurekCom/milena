package com.turek.milena.android

object MilenaTextChunker {
    private const val MAX_CHARS = 220

    fun chunk(text: String): List<String> {
        val normalized = text.replace(Regex("\\s+"), " ").trim()
        if (normalized.isEmpty()) {
            return emptyList()
        }
        if (normalized.length <= MAX_CHARS) {
            return listOf(normalized)
        }

        val result = mutableListOf<String>()
        val sentenceBuffer = StringBuilder()
        val sentences = normalized.split(Regex("(?<=[.!?;:])\\s+"))
        for (sentence in sentences) {
            val candidate = if (sentenceBuffer.isEmpty()) sentence else "${sentenceBuffer} $sentence"
            if (candidate.length <= MAX_CHARS) {
                sentenceBuffer.clear()
                sentenceBuffer.append(candidate)
                continue
            }

            if (sentenceBuffer.isNotEmpty()) {
                result += sentenceBuffer.toString().trim()
                sentenceBuffer.clear()
            }

            if (sentence.length <= MAX_CHARS) {
                sentenceBuffer.append(sentence)
                continue
            }

            splitLongSentence(sentence).forEach { result += it }
        }

        if (sentenceBuffer.isNotEmpty()) {
            result += sentenceBuffer.toString().trim()
        }
        return result.filter { it.isNotBlank() }
    }

    private fun splitLongSentence(sentence: String): List<String> {
        val chunks = mutableListOf<String>()
        val words = sentence.split(' ')
        val buffer = StringBuilder()
        for (word in words) {
            val candidate = if (buffer.isEmpty()) word else "${buffer} $word"
            if (candidate.length <= MAX_CHARS) {
                buffer.clear()
                buffer.append(candidate)
            } else {
                if (buffer.isNotEmpty()) {
                    chunks += buffer.toString().trim()
                    buffer.clear()
                }
                if (word.length <= MAX_CHARS) {
                    buffer.append(word)
                } else {
                    word.chunked(MAX_CHARS).forEach { chunks += it }
                }
            }
        }
        if (buffer.isNotEmpty()) {
            chunks += buffer.toString().trim()
        }
        return chunks
    }
}
