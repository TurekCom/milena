#include <windows.h>
#include <sapi.h>
#include <sapiddk.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cwctype>
#include <fstream>
#include <locale>
#include <new>
#include <sstream>
#include <string>
#include <vector>

namespace {

const CLSID CLSID_MilenaSapi5 =
{ 0x0e23aafa, 0x4f1d, 0x4a09, { 0xa8, 0xa4, 0x75, 0x9a, 0x84, 0xea, 0xc2, 0x19 } };

const GUID kSpdfidWaveFormatEx =
{ 0xC31ADBAE, 0x527F, 0x4FF5, { 0xA2, 0x30, 0xF6, 0x2B, 0xB6, 0x1F, 0xF7, 0x0C } };

constexpr wchar_t kEngineName[] = L"Milena SAPI5 Engine";
constexpr wchar_t kPerUserClsidRoot[] = L"Software\\Classes\\CLSID";
constexpr wchar_t kMilenaExeRelativePathValueName[] = L"MilenaExeRelativePath";
constexpr wchar_t kMbrolaExePathValueName[] = L"MbrolaExePath";
constexpr wchar_t kMbrolaVoicePathValueName[] = L"MbrolaVoicePath";
constexpr wchar_t kBaseTempoPercentValueName[] = L"BaseTempoPercent";
constexpr wchar_t kBasePitchPercentValueName[] = L"BasePitchPercent";
constexpr DWORD kSynthesisTimeoutMs = 60000;
constexpr uint32_t kSampleRate = 16000;

HMODULE g_module = nullptr;
std::atomic<ULONG> g_objects{0};
std::atomic<ULONG> g_locks{0};

struct RuntimeConfig {
    std::wstring milenaExeRelativePath = L"milena.exe";
    std::wstring mbrolaExePath;
    std::wstring mbrolaVoicePath;
    int baseTempoPercent = 100;
    int basePitchPercent = 100;
};

struct TempFiles {
    std::wstring base;
    std::wstring inputText;
    std::wstring pho;
    std::wstring raw;
};

bool IsSpeechAction(SPVACTIONS action) {
    return action == SPVA_Speak || action == SPVA_Pronounce || action == SPVA_SpellOut;
}

int ClampInt(int value, int minValue, int maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

double ClampDouble(double value, double minValue, double maxValue) {
    return std::max(minValue, std::min(maxValue, value));
}

bool IsWhitespace(wchar_t ch) {
    return ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n';
}

std::wstring CollapseWhitespace(const std::wstring& text) {
    std::wstring out;
    out.reserve(text.size());
    bool pendingSpace = false;
    for (wchar_t ch : text) {
        wchar_t normalized = ch;
        if (ch == L'\r' || ch == L'\n' || ch == L'\t') {
            normalized = L' ';
        }
        if (IsWhitespace(normalized)) {
            pendingSpace = !out.empty();
            continue;
        }
        if (pendingSpace) {
            out.push_back(L' ');
            pendingSpace = false;
        }
        out.push_back(normalized);
    }
    return out;
}

std::wstring SpellOutText(const wchar_t* text, ULONG len) {
    std::wstring out;
    for (ULONG i = 0; i < len; ++i) {
        if (i != 0) {
            out.push_back(L' ');
        }
        out.push_back(text[i]);
    }
    return out;
}

std::wstring JoinSpeakText(const SPVTEXTFRAG* fragList) {
    std::wstring out;
    for (auto frag = fragList; frag; frag = frag->pNext) {
        if (frag->pTextStart && frag->ulTextLen > 0 && IsSpeechAction(frag->State.eAction)) {
            if (frag->State.eAction == SPVA_SpellOut) {
                out += SpellOutText(frag->pTextStart, frag->ulTextLen);
            } else {
                out.append(frag->pTextStart, frag->ulTextLen);
            }
            out.push_back(L' ');
        } else if (frag->State.eAction == SPVA_Silence) {
            out.push_back(L' ');
        }
    }
    return CollapseWhitespace(out);
}

std::wstring GetModulePath() {
    wchar_t path[MAX_PATH] = {0};
    if (GetModuleFileNameW(g_module, path, static_cast<DWORD>(std::size(path))) == 0) {
        return {};
    }
    return std::wstring(path);
}

std::wstring GetDirectoryName(const std::wstring& path) {
    const std::wstring::size_type pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) {
        return {};
    }
    return path.substr(0, pos);
}

std::wstring JoinPath(const std::wstring& left, const std::wstring& right) {
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    const bool leftHasSlash = left.back() == L'\\' || left.back() == L'/';
    const bool rightHasSlash = right.front() == L'\\' || right.front() == L'/';
    if (leftHasSlash && rightHasSlash) {
        return left + right.substr(1);
    }
    if (leftHasSlash || rightHasSlash) {
        return left + right;
    }
    return left + L'\\' + right;
}

bool PathExists(const std::wstring& path) {
    const DWORD attrs = GetFileAttributesW(path.c_str());
    return attrs != INVALID_FILE_ATTRIBUTES;
}

bool IsAbsolutePath(const std::wstring& path) {
    if (path.size() >= 2 && path[1] == L':') {
        return true;
    }
    if (path.size() >= 2 && path[0] == L'\\' && path[1] == L'\\') {
        return true;
    }
    return false;
}

std::wstring ResolveInstallPath(const std::wstring& installRoot, const std::wstring& path) {
    if (path.empty() || IsAbsolutePath(path)) {
        return path;
    }
    return JoinPath(installRoot, path);
}

std::wstring GetShortPathIfAvailable(const std::wstring& path) {
    const DWORD needed = GetShortPathNameW(path.c_str(), nullptr, 0);
    if (needed == 0) {
        return path;
    }
    std::wstring out(needed, L'\0');
    const DWORD written = GetShortPathNameW(path.c_str(), out.data(), needed);
    if (written == 0) {
        return path;
    }
    out.resize(written);
    return out;
}

std::wstring FormatRatio(double value) {
    std::wostringstream stream;
    stream.imbue(std::locale::classic());
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(3);
    stream << value;
    return stream.str();
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int needed = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0) {
        return {};
    }
    std::string out(static_cast<size_t>(needed), '\0');
    const int written = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), needed, nullptr, nullptr);
    if (written <= 0) {
        return {};
    }
    return out;
}

bool WriteUtf8File(const std::wstring& path, const std::wstring& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        return false;
    }
    static const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    out.write(reinterpret_cast<const char*>(bom), sizeof(bom));
    const std::string utf8 = WideToUtf8(text);
    out.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
    return static_cast<bool>(out);
}

bool ReadFileBytes(const std::wstring& path, std::vector<uint8_t>* out) {
    if (!out) {
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return false;
    }
    in.seekg(0, std::ios::end);
    const std::streamoff size = in.tellg();
    if (size < 0) {
        return false;
    }
    in.seekg(0, std::ios::beg);
    out->assign(static_cast<size_t>(size), 0);
    if (size == 0) {
        return true;
    }
    in.read(reinterpret_cast<char*>(out->data()), size);
    return static_cast<bool>(in);
}

void ApplyVolumeToPcm(std::vector<uint8_t>* pcm, USHORT volume) {
    if (!pcm || pcm->empty()) {
        return;
    }
    const int level = ClampInt(static_cast<int>(volume), 0, 100);
    if (level >= 100) {
        return;
    }
    if (level <= 0) {
        std::fill(pcm->begin(), pcm->end(), static_cast<uint8_t>(0));
        return;
    }
    const double gain = static_cast<double>(level) / 100.0;
    for (size_t i = 0; i + 1 < pcm->size(); i += 2) {
        const uint16_t raw = static_cast<uint16_t>((*pcm)[i]) | (static_cast<uint16_t>((*pcm)[i + 1]) << 8);
        const int16_t sample = static_cast<int16_t>(raw);
        const int scaled = ClampInt(static_cast<int>(sample * gain), -32768, 32767);
        const uint16_t out = static_cast<uint16_t>(static_cast<int16_t>(scaled));
        (*pcm)[i] = static_cast<uint8_t>(out & 0xFF);
        (*pcm)[i + 1] = static_cast<uint8_t>((out >> 8) & 0xFF);
    }
}

bool MakeTempFiles(TempFiles* files) {
    if (!files) {
        return false;
    }
    wchar_t tempDir[MAX_PATH] = {0};
    if (GetTempPathW(static_cast<DWORD>(std::size(tempDir)), tempDir) == 0) {
        return false;
    }
    wchar_t tempFile[MAX_PATH] = {0};
    if (GetTempFileNameW(tempDir, L"mil", 0, tempFile) == 0) {
        return false;
    }
    DeleteFileW(tempFile);
    files->base = tempFile;
    files->inputText = files->base + L".txt";
    files->pho = files->base + L".pho";
    files->raw = files->base + L".raw";
    return true;
}

void CleanupTempFiles(const TempFiles& files) {
    if (!files.inputText.empty()) {
        DeleteFileW(files.inputText.c_str());
    }
    if (!files.pho.empty()) {
        DeleteFileW(files.pho.c_str());
    }
    if (!files.raw.empty()) {
        DeleteFileW(files.raw.c_str());
    }
}

bool ReadTokenString(ISpObjectToken* token, const wchar_t* name, std::wstring* outValue) {
    if (!token || !name || !outValue) {
        return false;
    }
    wchar_t* value = nullptr;
    const HRESULT hr = token->GetStringValue(name, &value);
    if (FAILED(hr) || !value || value[0] == L'\0') {
        if (value) {
            CoTaskMemFree(value);
        }
        return false;
    }
    *outValue = value;
    CoTaskMemFree(value);
    return true;
}

bool ReadTokenInt(ISpObjectToken* token, const wchar_t* name, int minValue, int maxValue, int* outValue) {
    std::wstring textValue;
    if (!ReadTokenString(token, name, &textValue)) {
        return false;
    }
    const int value = _wtoi(textValue.c_str());
    if (value < minValue || value > maxValue) {
        return false;
    }
    *outValue = value;
    return true;
}

RuntimeConfig LoadRuntimeConfig(ISpObjectToken* token) {
    RuntimeConfig config;
    if (!token) {
        return config;
    }
    std::wstring textValue;
    int intValue = 0;
    if (ReadTokenString(token, kMilenaExeRelativePathValueName, &textValue)) {
        config.milenaExeRelativePath = textValue;
    }
    if (ReadTokenString(token, kMbrolaExePathValueName, &textValue)) {
        config.mbrolaExePath = textValue;
    }
    if (ReadTokenString(token, kMbrolaVoicePathValueName, &textValue)) {
        config.mbrolaVoicePath = textValue;
    }
    if (ReadTokenInt(token, kBaseTempoPercentValueName, 25, 400, &intValue)) {
        config.baseTempoPercent = intValue;
    }
    if (ReadTokenInt(token, kBasePitchPercentValueName, 25, 400, &intValue)) {
        config.basePitchPercent = intValue;
    }
    return config;
}

std::wstring BuildCommandLine(const std::wstring& exePath, const std::vector<std::wstring>& args) {
    std::wstring commandLine = L"\"" + exePath + L"\"";
    for (const std::wstring& arg : args) {
        commandLine += L" ";
        commandLine += L"\"" + arg + L"\"";
    }
    return commandLine;
}

bool WaitForProcessOrAbort(HANDLE processHandle, ISpTTSEngineSite* site, DWORD timeoutMs, DWORD* exitCode) {
    const DWORD startTick = GetTickCount();
    for (;;) {
        const DWORD waitResult = WaitForSingleObject(processHandle, 100);
        if (waitResult == WAIT_OBJECT_0) {
            DWORD code = 1;
            if (!GetExitCodeProcess(processHandle, &code)) {
                return false;
            }
            if (exitCode) {
                *exitCode = code;
            }
            return true;
        }
        if (waitResult != WAIT_TIMEOUT) {
            return false;
        }
        if (site && (site->GetActions() & SPVES_ABORT) != 0) {
            TerminateProcess(processHandle, 1);
            return false;
        }
        if (GetTickCount() - startTick > timeoutMs) {
            TerminateProcess(processHandle, 1);
            return false;
        }
    }
}

bool RunProcessRedirected(
    const std::wstring& exePath,
    const std::vector<std::wstring>& args,
    const std::wstring& inputPath,
    const std::wstring& outputPath,
    ISpTTSEngineSite* site) {
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    HANDLE inputHandle = CreateFileW(inputPath.c_str(), GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (inputHandle == INVALID_HANDLE_VALUE) {
        return false;
    }
    HANDLE outputHandle = CreateFileW(outputPath.c_str(), GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (outputHandle == INVALID_HANDLE_VALUE) {
        CloseHandle(inputHandle);
        return false;
    }
    HANDLE nullHandle = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (nullHandle == INVALID_HANDLE_VALUE) {
        CloseHandle(outputHandle);
        CloseHandle(inputHandle);
        return false;
    }

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdInput = inputHandle;
    startupInfo.hStdOutput = outputHandle;
    startupInfo.hStdError = nullHandle;

    PROCESS_INFORMATION processInfo{};
    std::wstring commandLine = BuildCommandLine(exePath, args);
    const BOOL created = CreateProcessW(
        exePath.c_str(),
        commandLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo);

    CloseHandle(nullHandle);
    CloseHandle(outputHandle);
    CloseHandle(inputHandle);

    if (!created) {
        return false;
    }

    DWORD exitCode = 1;
    const bool ok = WaitForProcessOrAbort(processInfo.hProcess, site, kSynthesisTimeoutMs, &exitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return ok && exitCode == 0;
}

bool RunProcessSimple(
    const std::wstring& exePath,
    const std::vector<std::wstring>& args,
    ISpTTSEngineSite* site) {
    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};
    std::wstring commandLine = BuildCommandLine(exePath, args);

    const BOOL created = CreateProcessW(
        exePath.c_str(),
        commandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo);

    if (!created) {
        return false;
    }

    DWORD exitCode = 1;
    const bool ok = WaitForProcessOrAbort(processInfo.hProcess, site, kSynthesisTimeoutMs, &exitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return ok && exitCode == 0;
}

bool SynthesizeWithMilena(
    const std::wstring& text,
    long rateAdjust,
    USHORT volume,
    const RuntimeConfig& runtimeConfig,
    ISpTTSEngineSite* outputSite,
    std::vector<uint8_t>* pcm) {
    if (!pcm) {
        return false;
    }

    const std::wstring moduleDir = GetDirectoryName(GetModulePath());
    if (moduleDir.empty()) {
        return false;
    }
    const std::wstring installRoot = GetDirectoryName(moduleDir);
    const std::wstring milenaExe = ResolveInstallPath(installRoot, runtimeConfig.milenaExeRelativePath);
    const std::wstring mbrolaExe = ResolveInstallPath(installRoot, runtimeConfig.mbrolaExePath);
    const std::wstring mbrolaVoice = ResolveInstallPath(installRoot, runtimeConfig.mbrolaVoicePath);

    if (!PathExists(milenaExe) || !PathExists(mbrolaExe) || !PathExists(mbrolaVoice)) {
        return false;
    }

    TempFiles files;
    if (!MakeTempFiles(&files)) {
        return false;
    }
    if (!WriteUtf8File(files.inputText, text)) {
        CleanupTempFiles(files);
        return false;
    }

    const bool milenaOk = RunProcessRedirected(milenaExe, {L"-U"}, files.inputText, files.pho, outputSite);
    if (!milenaOk) {
        CleanupTempFiles(files);
        return false;
    }

    const int tempoPercent = ClampInt((runtimeConfig.baseTempoPercent * (100 - static_cast<int>(rateAdjust) * 8)) / 100, 25, 400);
    const int pitchPercent = ClampInt(runtimeConfig.basePitchPercent, 25, 400);

    std::vector<std::wstring> mbrolaArgs = {
        L"-e",
        L"-f", FormatRatio(static_cast<double>(pitchPercent) / 100.0),
        L"-t", FormatRatio(static_cast<double>(tempoPercent) / 100.0),
        GetShortPathIfAvailable(mbrolaVoice),
        files.pho,
        files.raw
    };

    const bool mbrolaOk = RunProcessSimple(GetShortPathIfAvailable(mbrolaExe), mbrolaArgs, outputSite);
    if (!mbrolaOk) {
        CleanupTempFiles(files);
        return false;
    }

    const bool readOk = ReadFileBytes(files.raw, pcm);
    if (readOk) {
        ApplyVolumeToPcm(pcm, volume);
    }
    CleanupTempFiles(files);
    return readOk;
}

bool IsPcm16Mono(const WAVEFORMATEX* fmt) {
    return fmt &&
        fmt->wFormatTag == WAVE_FORMAT_PCM &&
        fmt->nChannels == 1 &&
        fmt->wBitsPerSample == 16 &&
        fmt->nSamplesPerSec == kSampleRate &&
        fmt->nBlockAlign == 2 &&
        fmt->nAvgBytesPerSec == kSampleRate * 2;
}

HRESULT WriteRegString(HKEY root, const std::wstring& subKey, const wchar_t* name, const wchar_t* value) {
    HKEY key = nullptr;
    LONG rc = RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr, &key, nullptr);
    if (rc != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(rc);
    }
    const DWORD cb = static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t));
    rc = RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value), cb);
    RegCloseKey(key);
    return (rc == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(rc);
}

HRESULT DeleteRegTree(HKEY root, const std::wstring& subKey) {
    const LONG rc = RegDeleteTreeW(root, subKey.c_str());
    if (rc == ERROR_FILE_NOT_FOUND || rc == ERROR_PATH_NOT_FOUND) {
        return S_OK;
    }
    return (rc == ERROR_SUCCESS) ? S_OK : HRESULT_FROM_WIN32(rc);
}

HRESULT RegisterClsidInHive(HKEY root, REFCLSID clsidValue, const wchar_t* displayName, const wchar_t* modulePath) {
    wchar_t clsid[64] = {0};
    if (StringFromGUID2(clsidValue, clsid, static_cast<int>(std::size(clsid))) == 0) {
        return E_FAIL;
    }
    const std::wstring clsidKey = std::wstring(kPerUserClsidRoot) + L"\\" + clsid;
    HRESULT hr = WriteRegString(root, clsidKey, nullptr, displayName);
    if (FAILED(hr)) {
        return hr;
    }
    const std::wstring inprocKey = clsidKey + L"\\InprocServer32";
    hr = WriteRegString(root, inprocKey, nullptr, modulePath);
    if (FAILED(hr)) {
        return hr;
    }
    return WriteRegString(root, inprocKey, L"ThreadingModel", L"Both");
}

HRESULT UnregisterClsidInHive(HKEY root, REFCLSID clsidValue) {
    wchar_t clsid[64] = {0};
    if (StringFromGUID2(clsidValue, clsid, static_cast<int>(std::size(clsid))) == 0) {
        return E_FAIL;
    }
    const std::wstring clsidKey = std::wstring(kPerUserClsidRoot) + L"\\" + clsid;
    return DeleteRegTree(root, clsidKey);
}

class MilenaEngine final : public ISpTTSEngine, public ISpObjectWithToken {
public:
    MilenaEngine() : refCount_(1), token_(nullptr), runtimeConfig_() {
        g_objects.fetch_add(1, std::memory_order_relaxed);
    }

    ~MilenaEngine() {
        if (token_) {
            token_->Release();
            token_ = nullptr;
        }
        g_objects.fetch_sub(1, std::memory_order_relaxed);
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) {
            return E_POINTER;
        }
        *ppv = nullptr;
        if (riid == IID_IUnknown || riid == __uuidof(ISpTTSEngine)) {
            *ppv = static_cast<ISpTTSEngine*>(this);
        } else if (riid == __uuidof(ISpObjectWithToken)) {
            *ppv = static_cast<ISpObjectWithToken*>(this);
        } else {
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(reinterpret_cast<LONG*>(&refCount_)));
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG value = static_cast<ULONG>(InterlockedDecrement(reinterpret_cast<LONG*>(&refCount_)));
        if (value == 0) {
            delete this;
        }
        return value;
    }

    HRESULT STDMETHODCALLTYPE SetObjectToken(ISpObjectToken* token) override {
        if (token_) {
            token_->Release();
            token_ = nullptr;
        }
        runtimeConfig_ = RuntimeConfig{};
        if (token) {
            token->AddRef();
            token_ = token;
            runtimeConfig_ = LoadRuntimeConfig(token_);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetObjectToken(ISpObjectToken** ppToken) override {
        if (!ppToken) {
            return E_POINTER;
        }
        *ppToken = token_;
        if (token_) {
            token_->AddRef();
        }
        return token_ ? S_OK : S_FALSE;
    }

    HRESULT STDMETHODCALLTYPE Speak(DWORD, REFGUID, const WAVEFORMATEX*, const SPVTEXTFRAG* textFragList, ISpTTSEngineSite* outputSite) override {
        if (!outputSite) {
            return E_POINTER;
        }
        if ((outputSite->GetActions() & SPVES_ABORT) != 0) {
            return S_OK;
        }

        const std::wstring text = JoinSpeakText(textFragList);
        if (text.empty()) {
            return S_OK;
        }

        long rateAdjust = 0;
        (void)outputSite->GetRate(&rateAdjust);
        USHORT volume = 100;
        (void)outputSite->GetVolume(&volume);

        std::vector<uint8_t> pcm;
        if (!SynthesizeWithMilena(text, rateAdjust, volume, runtimeConfig_, outputSite, &pcm)) {
            if ((outputSite->GetActions() & SPVES_ABORT) != 0) {
                return S_OK;
            }
            return E_FAIL;
        }
        if (pcm.empty()) {
            return S_OK;
        }

        ULONG written = 0;
        return outputSite->Write(pcm.data(), static_cast<ULONG>(pcm.size()), &written);
    }

    HRESULT STDMETHODCALLTYPE GetOutputFormat(const GUID* targetFmtId, const WAVEFORMATEX* targetWaveFmt, GUID* outputFormatId, WAVEFORMATEX** ppCoMemOutputWaveFormatEx) override {
        if (!outputFormatId || !ppCoMemOutputWaveFormatEx) {
            return E_POINTER;
        }
        *ppCoMemOutputWaveFormatEx = nullptr;

        auto* fmt = static_cast<WAVEFORMATEX*>(CoTaskMemAlloc(sizeof(WAVEFORMATEX)));
        if (!fmt) {
            return E_OUTOFMEMORY;
        }

        if (targetFmtId && *targetFmtId == kSpdfidWaveFormatEx && IsPcm16Mono(targetWaveFmt)) {
            *fmt = *targetWaveFmt;
        } else {
            fmt->wFormatTag = WAVE_FORMAT_PCM;
            fmt->nChannels = 1;
            fmt->nSamplesPerSec = kSampleRate;
            fmt->wBitsPerSample = 16;
            fmt->nBlockAlign = static_cast<WORD>((fmt->nChannels * fmt->wBitsPerSample) / 8);
            fmt->nAvgBytesPerSec = fmt->nSamplesPerSec * fmt->nBlockAlign;
            fmt->cbSize = 0;
        }

        *outputFormatId = kSpdfidWaveFormatEx;
        *ppCoMemOutputWaveFormatEx = fmt;
        return S_OK;
    }

private:
    ULONG refCount_;
    ISpObjectToken* token_;
    RuntimeConfig runtimeConfig_;
};

class ClassFactory final : public IClassFactory {
public:
    ClassFactory() : refCount_(1) {}

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) {
            return E_POINTER;
        }
        *ppv = nullptr;
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(reinterpret_cast<LONG*>(&refCount_)));
    }

    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG value = static_cast<ULONG>(InterlockedDecrement(reinterpret_cast<LONG*>(&refCount_)));
        if (value == 0) {
            delete this;
        }
        return value;
    }

    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID riid, void** ppv) override {
        if (!ppv) {
            return E_POINTER;
        }
        *ppv = nullptr;
        if (outer) {
            return CLASS_E_NOAGGREGATION;
        }
        auto* engine = new (std::nothrow) MilenaEngine();
        if (!engine) {
            return E_OUTOFMEMORY;
        }
        const HRESULT hr = engine->QueryInterface(riid, ppv);
        engine->Release();
        return hr;
    }

    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override {
        if (lock) {
            g_locks.fetch_add(1, std::memory_order_relaxed);
        } else {
            g_locks.fetch_sub(1, std::memory_order_relaxed);
        }
        return S_OK;
    }

private:
    ULONG refCount_;
};

} // namespace

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = instance;
        DisableThreadLibraryCalls(instance);
    }
    return TRUE;
}

extern "C" HRESULT __stdcall DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    if (!ppv) {
        return E_POINTER;
    }
    *ppv = nullptr;
    if (rclsid != CLSID_MilenaSapi5) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    auto* factory = new (std::nothrow) ClassFactory();
    if (!factory) {
        return E_OUTOFMEMORY;
    }
    const HRESULT hr = factory->QueryInterface(riid, ppv);
    factory->Release();
    return hr;
}

extern "C" HRESULT __stdcall DllCanUnloadNow(void) {
    return (g_objects.load(std::memory_order_relaxed) == 0 && g_locks.load(std::memory_order_relaxed) == 0) ? S_OK : S_FALSE;
}

extern "C" HRESULT __stdcall DllRegisterServer(void) {
    const std::wstring modulePath = GetModulePath();
    if (modulePath.empty()) {
        return E_FAIL;
    }
    return RegisterClsidInHive(HKEY_CURRENT_USER, CLSID_MilenaSapi5, kEngineName, modulePath.c_str());
}

extern "C" HRESULT __stdcall DllUnregisterServer(void) {
    return UnregisterClsidInHive(HKEY_CURRENT_USER, CLSID_MilenaSapi5);
}
