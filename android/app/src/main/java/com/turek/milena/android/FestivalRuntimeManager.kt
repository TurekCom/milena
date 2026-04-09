package com.turek.milena.android

import android.content.Context
import android.util.Log
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.Locale
import java.util.concurrent.TimeUnit
import kotlin.math.roundToInt

data class SynthResult(
    val sampleRate: Int,
    val pcm: ByteArray,
)

object MilenaRuntimeManager {
    private const val TAG = "MilenaRuntime"
    private const val RUNTIME_VERSION = 1
    private const val PROCESS_TIMEOUT_SECONDS = 60L
    private val installLock = Any()

    @Volatile
    private var activeProcess: Process? = null

    private data class InstalledRuntime(
        val rootDir: File,
        val dataDir: File,
        val mbrolaDir: File,
    )

    fun runtimeStatus(context: Context): String {
        val runtime = ensureRuntimeInstalled(context)
        val abi = currentAbi(context) ?: "unknown"
        val milenaOk = milenaExecutable(context).exists()
        val mbrolaOk = mbrolaExecutable(context).exists()
        val voiceOk = File(runtime.mbrolaDir, "pl1").exists()
        return if (milenaOk && mbrolaOk && voiceOk) {
            "$abi, data=${runtime.dataDir.name}, voice=pl1"
        } else {
            "Brak kompletnego runtime dla ABI $abi"
        }
    }

    fun stopActiveSynthesis() {
        activeProcess?.destroy()
        activeProcess?.destroyForcibly()
        activeProcess = null
    }

    fun synthesize(
        context: Context,
        text: String,
        voice: MilenaVoiceSpec,
        ratePercent: Int,
        pitchPercent: Int,
        volumePercent: Int,
    ): SynthResult {
        val runtime = ensureRuntimeInstalled(context)
        val milenaExe = milenaExecutable(context)
        val mbrolaExe = mbrolaExecutable(context)
        val voiceDb = File(runtime.mbrolaDir, "pl1")

        require(milenaExe.exists()) { "Missing Milena executable for current ABI" }
        require(mbrolaExe.exists()) { "Missing MBROLA executable for current ABI" }
        require(voiceDb.exists()) { "Missing MBROLA voice database" }

        val workDir = File(context.cacheDir, "milena_synth").apply { mkdirs() }
        val stamp = System.nanoTime().toString()
        val txtFile = File(workDir, "milena_$stamp.txt")
        val phoFile = File(workDir, "milena_$stamp.pho")
        val rawFile = File(workDir, "milena_$stamp.raw")
        val milenaLogFile = File(workDir, "milena_$stamp.milena.log")
        val mbrolaLogFile = File(workDir, "milena_$stamp.mbrola.log")

        try {
            txtFile.writeText(text, Charsets.UTF_8)

            runProcess(
                processBuilder = ProcessBuilder(
                    milenaExe.absolutePath,
                    "-U",
                )
                    .directory(runtime.rootDir)
                    .redirectInput(txtFile)
                    .redirectOutput(phoFile)
                    .redirectError(milenaLogFile),
                environment = mapOf(
                    "HOME" to context.filesDir.absolutePath,
                    "TMPDIR" to context.cacheDir.absolutePath,
                ),
                failureLabel = "milena",
                logFile = milenaLogFile,
            )

            val tempoPercent = tempoPercentForRate(voice, ratePercent)
            val pitchScaledPercent = pitchPercentForVoice(voice, pitchPercent)
            val outputVolumePercent = effectiveVolumePercent(voice, volumePercent)

            runProcess(
                processBuilder = ProcessBuilder(
                    mbrolaExe.absolutePath,
                    "-e",
                    "-f",
                    formatRatio(pitchScaledPercent / 100.0),
                    "-t",
                    formatRatio(tempoPercent / 100.0),
                    voiceDb.absolutePath,
                    phoFile.absolutePath,
                    rawFile.absolutePath,
                )
                    .directory(runtime.rootDir)
                    .redirectInput(ProcessBuilder.Redirect.PIPE)
                    .redirectOutput(ProcessBuilder.Redirect.appendTo(mbrolaLogFile))
                    .redirectError(ProcessBuilder.Redirect.appendTo(mbrolaLogFile)),
                environment = mapOf(
                    "LD_LIBRARY_PATH" to (context.applicationInfo.nativeLibraryDir ?: ""),
                    "HOME" to context.filesDir.absolutePath,
                    "TMPDIR" to context.cacheDir.absolutePath,
                ),
                failureLabel = "mbrola",
                logFile = mbrolaLogFile,
            )

            val pcm = applyVolumeToPcm(rawFile.readBytes(), outputVolumePercent)
            return SynthResult(sampleRate = MilenaEngine.SAMPLE_RATE, pcm = pcm)
        } finally {
            activeProcess = null
            txtFile.delete()
            phoFile.delete()
            rawFile.delete()
            milenaLogFile.delete()
            mbrolaLogFile.delete()
        }
    }

    private fun ensureRuntimeInstalled(context: Context): InstalledRuntime {
        synchronized(installLock) {
            val root = File(context.filesDir, MilenaEngine.RUNTIME_ROOT_NAME)
            val runtimeRoot = File(root, "runtime")
            val dataDir = File(runtimeRoot, "data")
            val mbrolaDir = File(runtimeRoot, "mbrola")
            val stampFile = File(root, ".runtime_version")
            val installedVersion = if (stampFile.exists()) stampFile.readText().trim() else ""
            if (
                installedVersion == RUNTIME_VERSION.toString() &&
                File(dataDir, "pl_phraser.dat").exists() &&
                File(mbrolaDir, "pl1").exists()
            ) {
                return InstalledRuntime(runtimeRoot, dataDir, mbrolaDir)
            }

            root.deleteRecursively()
            runtimeRoot.mkdirs()
            copyAssetTree(context, "${MilenaEngine.RUNTIME_ASSET_PATH}/data", dataDir)
            copyAssetTree(context, "${MilenaEngine.RUNTIME_ASSET_PATH}/mbrola", mbrolaDir)
            stampFile.writeText(RUNTIME_VERSION.toString())
            return InstalledRuntime(runtimeRoot, dataDir, mbrolaDir)
        }
    }

    private fun effectiveVolumePercent(voice: MilenaVoiceSpec, volumePercent: Int): Int =
        ((voice.preset.baseVolumePercent * volumePercent.coerceIn(MilenaRanges.VOLUME_MIN, MilenaRanges.VOLUME_MAX)) / 100.0)
            .roundToInt()
            .coerceIn(0, 200)

    private fun tempoPercentForRate(voice: MilenaVoiceSpec, ratePercent: Int): Int =
        (((200 - ratePercent.coerceIn(MilenaRanges.RATE_MIN, MilenaRanges.RATE_MAX)).coerceAtLeast(25)) *
            voice.preset.baseTempoPercent / 100.0)
            .roundToInt()
            .coerceIn(25, 400)

    private fun pitchPercentForVoice(voice: MilenaVoiceSpec, pitchPercent: Int): Int =
        ((voice.preset.basePitchPercent * pitchPercent.coerceIn(MilenaRanges.PITCH_MIN, MilenaRanges.PITCH_MAX)) / 100.0)
            .roundToInt()
            .coerceIn(25, 400)

    private fun formatRatio(value: Double): String =
        String.format(Locale.US, "%.3f", value)

    private fun runProcess(
        processBuilder: ProcessBuilder,
        environment: Map<String, String>,
        failureLabel: String,
        logFile: File,
    ) {
        val env = processBuilder.environment()
        environment.forEach { (key, value) ->
            if (value.isNotBlank()) {
                env[key] = value
            }
        }

        val process = processBuilder.start()
        activeProcess = process
        val finished = process.waitFor(PROCESS_TIMEOUT_SECONDS, TimeUnit.SECONDS)
        if (!finished) {
            process.destroyForcibly()
            activeProcess = null
            throw IllegalStateException("$failureLabel timeout")
        }
        val exitCode = process.exitValue()
        activeProcess = null
        if (exitCode != 0) {
            val logTail = runCatching {
                logFile.takeIf { it.exists() }?.readText()?.takeLast(4000).orEmpty()
            }.getOrDefault("")
            Log.e(TAG, "$failureLabel failed rc=$exitCode log=$logTail")
            throw IllegalStateException("$failureLabel failed rc=$exitCode")
        }
    }

    private fun applyVolumeToPcm(pcm: ByteArray, volumePercent: Int): ByteArray {
        if (pcm.isEmpty()) {
            return ByteArray(0)
        }
        val level = volumePercent.coerceIn(0, 200)
        if (level == 100) {
            return pcm
        }
        if (level <= 0) {
            return ByteArray(pcm.size)
        }

        val gain = level / 100.0
        val input = ByteBuffer.wrap(pcm).order(ByteOrder.LITTLE_ENDIAN)
        val output = ByteBuffer.allocate(pcm.size).order(ByteOrder.LITTLE_ENDIAN)
        while (input.remaining() >= 2) {
            val sample = input.short.toInt()
            val scaled = (sample * gain).roundToInt().coerceIn(Short.MIN_VALUE.toInt(), Short.MAX_VALUE.toInt())
            output.putShort(scaled.toShort())
        }
        return output.array()
    }

    private fun milenaExecutable(context: Context): File =
        File(context.applicationInfo.nativeLibraryDir, MilenaEngine.MILENA_EXECUTABLE_NAME)

    private fun mbrolaExecutable(context: Context): File =
        File(context.applicationInfo.nativeLibraryDir, MilenaEngine.MBROLA_EXECUTABLE_NAME)

    private fun currentAbi(context: Context): String? =
        context.applicationInfo.nativeLibraryDir
            ?.takeIf { it.isNotBlank() }
            ?.let { File(it).name }

    private fun copyAssetTree(context: Context, assetPath: String, destination: File) {
        val children = context.assets.list(assetPath).orEmpty()
        if (children.isEmpty()) {
            destination.parentFile?.mkdirs()
            context.assets.open(assetPath).use { input ->
                destination.outputStream().use { output -> input.copyTo(output) }
            }
            return
        }

        destination.mkdirs()
        children.forEach { child ->
            copyAssetTree(context, "$assetPath/$child", File(destination, child))
        }
    }
}
