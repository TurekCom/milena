package com.turek.milena.android

import android.content.Context
import java.text.BreakIterator
import java.text.Normalizer
import java.util.Locale

object SpeechTextNormalizer {
    private val somePunctuation = setOf(
        '.'.code,
        ','.code,
        '!'.code,
        '?'.code,
    )
    private val mostPunctuation = somePunctuation + setOf(
        ':'.code,
        ';'.code,
        '-'.code,
        '('.code,
        ')'.code,
        '['.code,
        ']'.code,
        '"'.code,
        '\''.code,
        '/'.code,
        '\\'.code,
    )
    private val decimalMultiplierRegex = Regex("(?<![\\p{L}\\p{N}])(\\d+)[,.](\\d+)\\s*[x×](?![\\p{L}\\p{N}])")
    private val percentRegex = Regex("(?<![\\p{L}\\p{N}])(\\d+)\\s*%(?![\\p{L}\\p{N}])")
    private val parenthesizedSuffixRegex = Regex("(?iu)\\b([\\p{L}]{3,})\\(([\\p{L}]{1,6})\\)")
    private const val POLISH_LETTERS = "ąćęłńóśźżĄĆĘŁŃÓŚŹŻ"
    private val latinFallbackMap = mapOf(
        'ß' to "ss",
        'ẞ' to "SS",
        'æ' to "ae",
        'Æ' to "AE",
        'œ' to "oe",
        'Œ' to "OE",
        'ø' to "o",
        'Ø' to "O",
        'ð' to "d",
        'Ð' to "D",
        'þ' to "th",
        'Þ' to "TH",
        'đ' to "d",
        'Đ' to "D",
        'ħ' to "h",
        'Ħ' to "H",
        'ı' to "i",
        'ĸ' to "k",
        'ŉ' to "n",
        'ŋ' to "n",
        'Ŋ' to "N",
    )

    private val singleLabels = mapOf(
        "a" to "a",
        "ą" to "a z ogonkiem",
        "b" to "be",
        "c" to "ce",
        "ć" to "ci",
        "d" to "de",
        "e" to "e",
        "ę" to "e z ogonkiem",
        "f" to "ef",
        "g" to "gie",
        "h" to "ha",
        "i" to "i",
        "j" to "jot",
        "k" to "ka",
        "l" to "el",
        "ł" to "eł",
        "m" to "em",
        "n" to "en",
        "ń" to "eń",
        "o" to "o",
        "ó" to "u z kreską",
        "p" to "pe",
        "q" to "ku",
        "r" to "er",
        "s" to "es",
        "ś" to "si",
        "t" to "te",
        "u" to "u",
        "v" to "fał",
        "w" to "wu",
        "x" to "iks",
        "y" to "igrek",
        "z" to "zet",
        "ź" to "zi",
        "ż" to "żet",
        "0" to "zero",
        "1" to "jeden",
        "2" to "dwa",
        "3" to "trzy",
        "4" to "cztery",
        "5" to "pięć",
        "6" to "sześć",
        "7" to "siedem",
        "8" to "osiem",
        "9" to "dziewięć",
        " " to "spacja",
        "\t" to "tabulator",
        "\n" to "enter",
        "\r" to "enter",
        "." to "kropka",
        "," to "przecinek",
        ":" to "dwukropek",
        ";" to "średnik",
        "!" to "wykrzyknik",
        "?" to "znak zapytania",
        "@" to "małpa",
        "#" to "kratka",
        "$" to "dolar",
        "%" to "procent",
        "^" to "daszek",
        "&" to "ampersand",
        "*" to "gwiazdka",
        "(" to "nawias otwierający",
        ")" to "nawias zamykający",
        "[" to "lewy nawias kwadratowy",
        "]" to "prawy nawias kwadratowy",
        "{" to "lewa klamra",
        "}" to "prawa klamra",
        "<" to "mniejsze niż",
        ">" to "większe niż",
        "/" to "ukośnik",
        "\\" to "ukośnik wsteczny",
        "|" to "pionowa kreska",
        "-" to "minus",
        "_" to "podkreślnik",
        "=" to "równa się",
        "+" to "plus",
        "\"" to "cudzysłów",
        "'" to "apostrof",
        "`" to "akcent odwrotny",
        "~" to "tylda",
    )

    private val asciiEmoticons = linkedMapOf(
        "<3" to "serce",
        ":-)" to "uśmiechnięta buźka",
        ":)" to "uśmiechnięta buźka",
        ":-(" to "smutna buźka",
        ":(" to "smutna buźka",
        ";-)" to "mrugająca buźka",
        ";)" to "mrugająca buźka",
        ":-D" to "roześmiana buźka",
        ":D" to "roześmiana buźka",
        ":-P" to "buźka z językiem",
        ":P" to "buźka z językiem",
        ":-*" to "buziak",
        ":*" to "buziak",
    )

    private val fallbackEmojiLabels = mapOf(
        "😀" to "uśmiechnięta buźka",
        "😃" to "szeroki uśmiech",
        "😄" to "uśmiechnięta buźka z otwartymi ustami",
        "😁" to "roześmiana buźka",
        "😂" to "buźka ze łzami radości",
        "🤣" to "tarza się ze śmiechu",
        "🙂" to "lekko uśmiechnięta buźka",
        "🙃" to "odwrócona buźka",
        "😉" to "mrugająca buźka",
        "😊" to "uśmiechnięta buźka z rumieńcami",
        "😍" to "buźka z sercami w oczach",
        "🥰" to "uśmiechnięta buźka z sercami",
        "😘" to "buźka wysyłająca buziaka",
        "😎" to "buźka w okularach",
        "😢" to "płacząca buźka",
        "😭" to "głośno płacząca buźka",
        "😡" to "wściekła buźka",
        "🤔" to "zamyślona buźka",
        "🙄" to "przewracanie oczami",
        "😐" to "neutralna buźka",
        "😑" to "buźka bez wyrazu",
        "😅" to "uśmiech z potem",
        "😇" to "uśmiechnięta buźka z aureolą",
        "😋" to "oblizująca się buźka",
        "😜" to "mrugająca buźka z językiem",
        "😴" to "śpiąca buźka",
        "🤖" to "robot",
        "💩" to "kupka",
        "👍" to "kciuk w górę",
        "👎" to "kciuk w dół",
        "👏" to "klaskanie",
        "🙏" to "złożone dłonie",
        "💪" to "napięty biceps",
        "❤" to "czerwone serce",
        "💔" to "złamane serce",
        "💯" to "sto punktów",
        "🔥" to "ogień",
        "🎉" to "konfetti",
        "✨" to "iskry",
        "⭐" to "gwiazda",
        "🌟" to "świecąca gwiazda",
        "✅" to "zielony znacznik wyboru",
        "❌" to "krzyżyk",
        "⚠" to "ostrzeżenie",
        "💬" to "dymek rozmowy",
        "📧" to "e-mail",
        "📞" to "telefon",
        "☺" to "uśmiechnięta buźka",
        "☹" to "smutna buźka",
    )

    private val skinToneLabels = mapOf(
        0x1F3FB to "jasna karnacja",
        0x1F3FC to "średnio jasna karnacja",
        0x1F3FD to "średnia karnacja",
        0x1F3FE to "średnio ciemna karnacja",
        0x1F3FF to "ciemna karnacja",
    )
    private val repeatCountLabels = mapOf(
        2 to "dwa razy",
        3 to "trzy razy",
        4 to "cztery razy",
        5 to "pięć razy",
        6 to "sześć razy",
        7 to "siedem razy",
        8 to "osiem razy",
    )

    fun normalize(
        context: Context,
        text: String,
        speakEmoji: Boolean,
        punctuationVerbosity: PunctuationVerbosity = PunctuationVerbosity.NONE,
    ): String {
        return normalize(text, speakEmoji, punctuationVerbosity, EmojiAnnotationsRepository.load(context))
    }

    fun normalize(text: String, speakEmoji: Boolean): String {
        return normalize(text, speakEmoji, PunctuationVerbosity.NONE, emptyMap())
    }

    fun normalize(text: String, speakEmoji: Boolean, punctuationVerbosity: PunctuationVerbosity): String {
        return normalize(text, speakEmoji, punctuationVerbosity, emptyMap())
    }

    fun normalize(text: String, speakEmoji: Boolean, emojiLabels: Map<String, String>): String {
        return normalize(text, speakEmoji, PunctuationVerbosity.NONE, emojiLabels)
    }

    fun normalize(
        text: String,
        speakEmoji: Boolean,
        punctuationVerbosity: PunctuationVerbosity,
        emojiLabels: Map<String, String>,
    ): String {
        if (text.isEmpty()) {
            return text
        }
        val preprocessed = preprocessUiText(text, punctuationVerbosity)
        val asciiNormalized = if (speakEmoji) replaceAsciiEmoticons(preprocessed) else preprocessed
        val clusters = graphemeClusters(asciiNormalized)
        if (clusters.size == 1) {
            return labelForSingleCluster(clusters.first(), speakEmoji, emojiLabels)
                ?: ensureSpeakableText(text, asciiNormalized)
        }
        if (!speakEmoji) {
            return ensureSpeakableText(text, asciiNormalized)
        }

        val out = StringBuilder(asciiNormalized.length + 32)
        var lastWasEmojiWord = false
        var index = 0
        while (index < clusters.size) {
            val cluster = clusters[index]
            val emojiLabel = emojiLabelForCluster(cluster, emojiLabels)
            if (emojiLabel != null) {
                var nextIndex = index + 1
                while (nextIndex < clusters.size && emojiLabelForCluster(clusters[nextIndex], emojiLabels) == emojiLabel) {
                    nextIndex++
                }
                val repeatCount = nextIndex - index
                appendSpacer(out)
                if (repeatCount > 1) {
                    out.append(repeatCountLabel(repeatCount))
                    out.append(' ')
                }
                out.append(emojiLabel)
                lastWasEmojiWord = true
                index = nextIndex
                continue
            }
            if (lastWasEmojiWord && needsLeadingSpace(cluster)) {
                out.append(' ')
            }
            out.append(cluster)
            lastWasEmojiWord = false
            index++
        }
        return ensureSpeakableText(text, out.toString().replace(Regex(" {2,}"), " ").trim())
    }

    private fun preprocessUiText(text: String, punctuationVerbosity: PunctuationVerbosity): String {
        val normalizedWhitespace = normalizeWhitespaceAndControls(text)
        val normalizedPunctuation = normalizeUiPunctuation(normalizedWhitespace)
        val simplifiedUiGrammar = simplifyUiGrammar(normalizedPunctuation)
        val condensedUiLabels = condenseVerboseUiLabels(simplifiedUiGrammar)
        val multiplierExpanded = decimalMultiplierRegex.replace(condensedUiLabels) { match ->
            "${speakNumberFragment(match.groupValues[1])} przecinek ${speakNumberFragment(match.groupValues[2])} razy"
        }
        val percentExpanded = percentRegex.replace(multiplierExpanded) { match ->
            "${speakNumberFragment(match.groupValues[1])} procent"
        }
        val separatorsNormalized = percentExpanded
            .replace(Regex("\\s*\\|\\s*"), ", ")
            .replace(Regex("\\s+/\\s+"), ", ")
            .replace(Regex("\\s+-\\s+"), ", ")
            .replace("×", " razy ")
        val quoteNormalized = separatorsNormalized
            .replace("\"", " ")
            .replace("'", " ")
            .replace(Regex("\\.{2,}"), ". ")
        val technicalExpanded = expandTechnicalTokens(quoteNormalized)
        val simplifiedScripts = simplifyForeignScripts(technicalExpanded)
        return applyPunctuationVerbosity(simplifiedScripts, punctuationVerbosity)
            .replace(Regex(" {2,}"), " ")
            .replace(Regex("\\s+,\\s*"), ", ")
            .replace(Regex("\\s+\\.\\s*"), ". ")
            .trim()
    }

    internal fun makeFestivalFriendly(
        text: String,
        punctuationVerbosity: PunctuationVerbosity = PunctuationVerbosity.NONE,
    ): String =
        ensureSpeakableText(text, preprocessUiText(text, punctuationVerbosity))
            .replace(Regex(" {2,}"), " ")
            .trim()

    internal fun ensureSpeakableText(original: String, normalized: String): String {
        if (normalized.isNotBlank()) {
            return normalized
        }
        return fallbackForUnreadableText(original)
    }

    internal fun fallbackForUnreadableText(original: String): String {
        if (original.isBlank()) {
            return ""
        }
        var sawLetter = false
        var sawNonLatinLetter = false
        var sawDigit = false
        var sawSymbol = false
        forEachCodePoint(original) { codePoint ->
            when {
                Character.isLetter(codePoint) -> {
                    sawLetter = true
                    val script = Character.UnicodeScript.of(codePoint)
                    if (script != Character.UnicodeScript.LATIN && !isPreservedPolishCodePoint(codePoint)) {
                        sawNonLatinLetter = true
                    }
                }
                Character.isDigit(codePoint) -> sawDigit = true
                isEmojiSupportCodePoint(codePoint) || isGenericSafePunctuation(codePoint) -> sawSymbol = true
            }
        }
        return when {
            sawNonLatinLetter -> "tekst w innym języku"
            sawLetter -> "tekst"
            sawDigit -> "liczba"
            sawSymbol -> "symbol"
            else -> "element"
        }
    }

    internal fun labelForSingleCluster(
        cluster: String,
        speakEmoji: Boolean,
        emojiLabels: Map<String, String> = emptyMap(),
    ): String? {
        if (cluster.isEmpty()) {
            return null
        }
        val normalizedCluster = Normalizer.normalize(cluster, Normalizer.Form.NFC)
        val labelKey = normalizedCluster.lowercase(Locale.ROOT)
        singleLabels[labelKey]?.let { return it }
        if (speakEmoji) {
            emojiLabelForCluster(normalizedCluster, emojiLabels)?.let { return it }
        }
        return null
    }

    private fun normalizeWhitespaceAndControls(text: String): String {
        val out = StringBuilder(text.length + 16)
        var index = 0
        while (index < text.length) {
            val codePoint = text.codePointAt(index)
            when {
                codePoint == '\r'.code -> {
                    // Normalize CRLF as a single spoken pause.
                }
                codePoint == '\n'.code -> out.append(" . ")
                codePoint == '\t'.code -> out.append(' ')
                codePoint == 0x200D -> out.appendCodePoint(codePoint)
                Character.getType(codePoint) == Character.FORMAT.toInt() -> {
                    // Directional marks and similar formatting controls should not split words.
                }
                Character.getType(codePoint) == Character.LINE_SEPARATOR.toInt() ||
                    Character.getType(codePoint) == Character.PARAGRAPH_SEPARATOR.toInt() ||
                    Character.getType(codePoint) == Character.SPACE_SEPARATOR.toInt() -> out.append(' ')
                else -> out.appendCodePoint(codePoint)
            }
            index += Character.charCount(codePoint)
        }
        return out.toString()
    }

    private fun normalizeUiPunctuation(text: String): String {
        val out = StringBuilder(text.length + 16)
        var index = 0
        while (index < text.length) {
            val codePoint = text.codePointAt(index)
            when (codePoint) {
                0x2010, 0x2011, 0x2012, 0x2013, 0x2014, 0x2015, 0x2212 -> out.append('-')
                0x2026 -> out.append("...")
                0x2022, 0x00B7, 0x2027, 0x2219, 0x2043, 0x25CF, 0x30FB -> {
                    if (out.isNotEmpty() && !out.last().isWhitespace()) {
                        out.append(", ")
                    } else {
                        out.append(", ")
                    }
                }
                0x2018, 0x2019, 0x201A, 0x201B, 0x2032, 0x02BC -> out.append('\'')
                0x201C, 0x201D, 0x201E, 0x201F, 0x2033 -> out.append('"')
                else -> out.appendCodePoint(codePoint)
            }
            index += Character.charCount(codePoint)
        }
        return out.toString()
    }

    private fun simplifyUiGrammar(text: String): String {
        return parenthesizedSuffixRegex.replace(text) { match ->
            match.groupValues[1]
        }
    }

    private fun condenseVerboseUiLabels(text: String): String {
        return text
            .replace("Zarządzaj ustawieniami powiadomień:", "Ustawienia powiadomień,")
            .replace("opcje odkładania powiadomień", "opcje powiadomień")
            .replace("Wycisz powiadomienia", "wycisz powiadomienia")
            .replace("Ikona aplikacji", "ikona aplikacji")
            .replace(Regex("(?iu)\\b(\\d+)\\s*min\\b"), "$1 minut")
    }

    private fun applyPunctuationVerbosity(text: String, punctuationVerbosity: PunctuationVerbosity): String {
        if (text.isEmpty()) {
            return text
        }
        if (punctuationVerbosity == PunctuationVerbosity.NONE) {
            return text
        }
        val out = StringBuilder(text.length + 24)
        var index = 0
        while (index < text.length) {
            val codePoint = text.codePointAt(index)
            val label = spokenPunctuationLabel(codePoint)
            if (label != null) {
                if (isEmbeddedWordNoise(text, index)) {
                    // Skip stray punctuation embedded inside a token so the rest of the word remains speakable.
                } else if (shouldSpeakPunctuation(codePoint, punctuationVerbosity)) {
                    if (out.isNotEmpty() && !out.last().isWhitespace()) {
                        out.append(' ')
                    }
                    out.append(label)
                    out.append(' ')
                } else {
                    out.appendCodePoint(codePoint)
                }
            } else {
                out.appendCodePoint(codePoint)
            }
            index += Character.charCount(codePoint)
        }
        return out.toString()
    }

    private fun simplifyForeignScripts(text: String): String {
        val out = StringBuilder(text.length + 16)
        var index = 0
        while (index < text.length) {
            val codePoint = text.codePointAt(index)
            when {
                codePoint <= 0x7F -> out.appendCodePoint(codePoint)
                isPreservedPolishCodePoint(codePoint) -> out.appendCodePoint(codePoint)
                isEmojiSupportCodePoint(codePoint) -> out.appendCodePoint(codePoint)
                Character.isWhitespace(codePoint) -> out.append(' ')
                isGenericSafePunctuation(codePoint) -> out.appendCodePoint(codePoint)
                Character.UnicodeScript.of(codePoint) == Character.UnicodeScript.LATIN -> {
                    appendLatinFallback(out, codePoint)
                }
                else -> {
                    if (!isEmbeddedWordNoise(text, index)) {
                        out.append(' ')
                    }
                }
            }
            index += Character.charCount(codePoint)
        }
        return out.toString()
    }

    private fun speakNumberFragment(fragment: String): String {
        if (fragment.isEmpty()) {
            return fragment
        }
        if (fragment.length == 1) {
            return singleLabels[fragment] ?: fragment
        }
        return fragment.toCharArray()
            .joinToString(" ") { singleLabels[it.toString()] ?: it.toString() }
    }

    private fun repeatCountLabel(count: Int): String =
        repeatCountLabels[count] ?: "$count razy"

    private fun expandTechnicalTokens(text: String): String =
        text.split(Regex("\\s+"))
            .filter { it.isNotEmpty() }
            .joinToString(" ") { token ->
                if (shouldExpandTechnicalToken(token)) expandTechnicalToken(token) else token
            }

    private fun shouldExpandTechnicalToken(token: String): Boolean {
        if (token.isEmpty()) {
            return false
        }
        if (token.contains("://")) {
            return true
        }
        if (token.startsWith('.') || token.startsWith('$')) {
            return true
        }
        if (token.any { it in "@\\_$*=#~" }) {
            return true
        }
        if (token.contains('/')) {
            return true
        }
        if (token.contains('\\')) {
            return true
        }
        val dotCount = token.count { it == '.' }
        if (dotCount == 0) {
            return false
        }
        if (dotCount >= 2) {
            return true
        }
        val dotIndex = token.indexOf('.')
        val hasDigit = token.any { it.isDigit() }
        val hasInnerDot = dotIndex in 1 until token.lastIndex
        return hasDigit || hasInnerDot
    }

    private fun expandTechnicalToken(token: String): String {
        val out = StringBuilder(token.length + 16)
        val current = StringBuilder()

        fun flushCurrent() {
            if (current.isNotEmpty()) {
                if (out.isNotEmpty() && !out.last().isWhitespace()) {
                    out.append(' ')
                }
                out.append(current)
                current.setLength(0)
            }
        }

        var index = 0
        while (index < token.length) {
            val codePoint = token.codePointAt(index)
            val label = technicalSymbolLabel(codePoint)
            if (label != null) {
                flushCurrent()
                if (out.isNotEmpty() && !out.last().isWhitespace()) {
                    out.append(' ')
                }
                out.append(label)
                out.append(' ')
            } else {
                current.appendCodePoint(codePoint)
            }
            index += Character.charCount(codePoint)
        }
        flushCurrent()
        return out.toString().replace(Regex(" {2,}"), " ").trim()
    }

    private fun technicalSymbolLabel(codePoint: Int): String? = when (codePoint) {
        '.'.code -> "kropka"
        ':'.code -> "dwukropek"
        ';'.code -> "średnik"
        '/'.code -> "ukośnik"
        '\\'.code -> "ukośnik wsteczny"
        '['.code -> "lewy nawias kwadratowy"
        ']'.code -> "prawy nawias kwadratowy"
        '('.code -> "nawias otwierający"
        ')'.code -> "nawias zamykający"
        '@'.code -> "małpa"
        '_'.code -> "podkreślnik"
        '-'.code -> "minus"
        '$'.code -> "dolar"
        '*'.code -> "gwiazdka"
        '='.code -> "równa się"
        '#'.code -> "kratka"
        '~'.code -> "tylda"
        '%'.code -> "procent"
        '×'.code -> "razy"
        else -> null
    }

    private fun spokenPunctuationLabel(codePoint: Int): String? =
        technicalSymbolLabel(codePoint)
            ?: when (codePoint) {
                ','.code -> "przecinek"
                '!'.code -> "wykrzyknik"
                '?'.code -> "znak zapytania"
                '+'.code -> "plus"
                '<'.code -> "mniejsze niż"
                '>'.code -> "większe niż"
                '{'.code -> "lewa klamra"
                '}'.code -> "prawa klamra"
                '&'.code -> "ampersand"
                '^'.code -> "daszek"
                '|'.code -> "pionowa kreska"
                else -> null
            }

    private fun shouldSpeakPunctuation(codePoint: Int, punctuationVerbosity: PunctuationVerbosity): Boolean =
        when (punctuationVerbosity) {
            PunctuationVerbosity.NONE -> false
            PunctuationVerbosity.SOME -> codePoint in somePunctuation
            PunctuationVerbosity.MOST -> codePoint in mostPunctuation
            PunctuationVerbosity.ALL -> spokenPunctuationLabel(codePoint) != null
        }

    private fun isPreservedPolishCodePoint(codePoint: Int): Boolean =
        codePoint <= Char.MAX_VALUE.code && POLISH_LETTERS.indexOf(codePoint.toChar()) >= 0

    private fun isGenericSafePunctuation(codePoint: Int): Boolean {
        return when (Character.getType(codePoint)) {
            Character.CONNECTOR_PUNCTUATION.toInt(),
            Character.DASH_PUNCTUATION.toInt(),
            Character.START_PUNCTUATION.toInt(),
            Character.END_PUNCTUATION.toInt(),
            Character.OTHER_PUNCTUATION.toInt(),
            Character.INITIAL_QUOTE_PUNCTUATION.toInt(),
            Character.FINAL_QUOTE_PUNCTUATION.toInt(),
            Character.MATH_SYMBOL.toInt(),
            Character.CURRENCY_SYMBOL.toInt() -> true
            else -> false
        }
    }

    private fun isEmojiSupportCodePoint(codePoint: Int): Boolean {
        return codePoint in 0x1F000..0x1FAFF ||
            codePoint in 0x1F1E6..0x1F1FF ||
            codePoint in 0x2600..0x27BF ||
            codePoint in 0x2300..0x23FF ||
            codePoint in 0x1F3FB..0x1F3FF ||
            codePoint == 0x00A9 ||
            codePoint == 0x00AE ||
            codePoint == 0x20E3 ||
            codePoint == 0x200D ||
            codePoint == 0xFE0F
    }

    private fun appendLatinFallback(out: StringBuilder, codePoint: Int) {
        if (codePoint <= Char.MAX_VALUE.code) {
            latinFallbackMap[codePoint.toChar()]?.let {
                out.append(it)
                return
            }
        }
        val normalized = Normalizer.normalize(String(Character.toChars(codePoint)), Normalizer.Form.NFD)
        var appended = false
        for (ch in normalized) {
            when {
                POLISH_LETTERS.indexOf(ch) >= 0 -> {
                    out.append(ch)
                    appended = true
                }
                Character.getType(ch) == Character.NON_SPACING_MARK.toInt() ||
                    Character.getType(ch) == Character.COMBINING_SPACING_MARK.toInt() ||
                    Character.getType(ch) == Character.ENCLOSING_MARK.toInt() -> {
                    // Skip diacritics. For non-Polish Latin text this simplifies pronunciation.
                }
                ch.code <= 0x7F -> {
                    out.append(ch)
                    appended = true
                }
            }
        }
        if (!appended) {
            out.append(' ')
        }
    }

    private fun isEmbeddedWordNoise(text: String, index: Int): Boolean {
        val previous = previousAdjacentWordishCodePoint(text, index)
        val current = text.codePointAt(index)
        val next = nextAdjacentWordishCodePoint(text, index + Character.charCount(current))
        return previous != null && next != null
    }

    private fun previousAdjacentWordishCodePoint(text: String, fromIndex: Int): Int? {
        var cursor = fromIndex
        while (cursor > 0) {
            val codePoint = text.codePointBefore(cursor)
            cursor -= Character.charCount(codePoint)
            if (Character.getType(codePoint) == Character.FORMAT.toInt()) {
                continue
            }
            if (Character.isWhitespace(codePoint)) {
                return null
            }
            return codePoint.takeIf(::isWordishCodePoint)
        }
        return null
    }

    private fun nextAdjacentWordishCodePoint(text: String, fromIndex: Int): Int? {
        var cursor = fromIndex
        while (cursor < text.length) {
            val codePoint = text.codePointAt(cursor)
            cursor += Character.charCount(codePoint)
            if (Character.getType(codePoint) == Character.FORMAT.toInt()) {
                continue
            }
            if (Character.isWhitespace(codePoint)) {
                return null
            }
            return codePoint.takeIf(::isWordishCodePoint)
        }
        return null
    }

    private fun isWordishCodePoint(codePoint: Int?): Boolean {
        if (codePoint == null) {
            return false
        }
        return Character.isLetterOrDigit(codePoint) ||
            isPreservedPolishCodePoint(codePoint) ||
            Character.UnicodeScript.of(codePoint) == Character.UnicodeScript.LATIN
    }

    private fun replaceAsciiEmoticons(text: String): String {
        var out = text
        for ((emoticon, spoken) in asciiEmoticons) {
            out = out.replace(emoticon, " $spoken ")
        }
        return out
    }

    private fun graphemeClusters(text: String): List<String> {
        val iterator = BreakIterator.getCharacterInstance(Locale.ROOT)
        iterator.setText(text)
        val out = ArrayList<String>(text.length.coerceAtLeast(1))
        var start = iterator.first()
        var end = iterator.next()
        while (end != BreakIterator.DONE) {
            out += text.substring(start, end)
            start = end
            end = iterator.next()
        }
        return out
    }

    private fun emojiLabelForCluster(cluster: String, emojiLabels: Map<String, String>): String? {
        emojiLabels[cluster]?.let { return it }
        val simplified = simplifyEmojiKey(cluster)
        emojiLabels[simplified]?.let { return it }
        fallbackEmojiLabels[simplified]?.let { base ->
            return withSkinTone(base, cluster)
        }
        keycapBaseLabel(cluster)?.let { return it }
        if (isFlagCluster(cluster)) {
            return "flaga"
        }
        if (isEmojiCluster(cluster)) {
            return "emotikona"
        }
        return null
    }

    private fun simplifyEmojiKey(cluster: String): String {
        val out = StringBuilder(cluster.length)
        forEachCodePoint(cluster) { codePoint ->
            if (codePoint == 0xFE0F || codePoint in 0x1F3FB..0x1F3FF) {
                return@forEachCodePoint
            }
            out.appendCodePoint(codePoint)
        }
        return out.toString()
    }

    private fun keycapBaseLabel(cluster: String): String? {
        if (!cluster.contains('\u20E3')) {
            return null
        }
        val simplified = simplifyEmojiKey(cluster)
        val base = simplified.firstOrNull { it.isDigit() || it == '#' || it == '*' } ?: return null
        return singleLabels[base.toString()]
    }

    private fun withSkinTone(base: String, cluster: String): String {
        val tone = extractSkinTone(cluster) ?: return base
        return "$base, $tone"
    }

    private fun extractSkinTone(cluster: String): String? {
        var found: String? = null
        forEachCodePoint(cluster) { codePoint ->
            skinToneLabels[codePoint]?.let { found = it }
        }
        return found
    }

    private fun isFlagCluster(cluster: String): Boolean {
        var count = 0
        var allRegionalIndicators = true
        forEachCodePoint(cluster) { codePoint ->
            count += 1
            if (codePoint !in 0x1F1E6..0x1F1FF) {
                allRegionalIndicators = false
            }
        }
        return allRegionalIndicators && count == 2
    }

    private fun isEmojiCluster(cluster: String): Boolean {
        var hasEmoji = false
        forEachCodePoint(cluster) { codePoint ->
            if (
                codePoint in 0x1F000..0x1FAFF ||
                codePoint in 0x2600..0x27BF ||
                codePoint in 0x2300..0x23FF ||
                codePoint in 0x1F1E6..0x1F1FF ||
                codePoint == 0x00A9 ||
                codePoint == 0x00AE ||
                codePoint == 0x20E3 ||
                codePoint == 0x200D ||
                codePoint == 0xFE0F
            ) {
                hasEmoji = true
            }
        }
        return hasEmoji
    }

    private fun appendSpacer(out: StringBuilder) {
        if (out.isNotEmpty() && !out.last().isWhitespace()) {
            out.append(' ')
        }
    }

    private fun needsLeadingSpace(cluster: String): Boolean {
        if (cluster.isEmpty()) {
            return false
        }
        val codePoint = cluster.codePointAt(0)
        if (Character.isWhitespace(codePoint)) {
            return false
        }
        return !isPunctuationLike(codePoint)
    }

    private fun isPunctuationLike(codePoint: Int): Boolean {
        return when (Character.getType(codePoint)) {
            Character.CONNECTOR_PUNCTUATION.toInt(),
            Character.DASH_PUNCTUATION.toInt(),
            Character.START_PUNCTUATION.toInt(),
            Character.END_PUNCTUATION.toInt(),
            Character.OTHER_PUNCTUATION.toInt(),
            Character.INITIAL_QUOTE_PUNCTUATION.toInt(),
            Character.FINAL_QUOTE_PUNCTUATION.toInt(),
            Character.MATH_SYMBOL.toInt(),
            Character.CURRENCY_SYMBOL.toInt(),
            Character.MODIFIER_SYMBOL.toInt(),
            Character.OTHER_SYMBOL.toInt() -> true
            else -> false
        }
    }

    private inline fun forEachCodePoint(text: String, block: (Int) -> Unit) {
        var index = 0
        while (index < text.length) {
            val codePoint = text.codePointAt(index)
            block(codePoint)
            index += Character.charCount(codePoint)
        }
    }
}
