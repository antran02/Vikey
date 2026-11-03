#include "Macro.h"
#include "Prefs.h"
#include <windows.h>
#include <algorithm>
#include <cwctype>
#include <sstream>

namespace vikey { namespace macro {

namespace {

std::wstring trim(const std::wstring& s) {
    size_t begin = s.find_first_not_of(L" \t\r\n");
    if (begin == std::wstring::npos) return std::wstring();
    size_t end = s.find_last_not_of(L" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

bool equalsNoCase(const std::wstring& a, const std::wstring& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (std::towlower(a[i]) != std::towlower(b[i])) return false;
    return true;
}

std::wstring decodeEscapes(const std::wstring& s) {
    std::wstring out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == L'\\' && i + 1 < s.size()) {
            wchar_t next = s[i + 1];
            if (next == L'n') { out += L'\n'; ++i; }
            else if (next == L't') { out += L'\t'; ++i; }
            else if (next == L'\\') { out += L'\\'; ++i; }
            else out += s[i];
        } else {
            out += s[i];
        }
    }
    return out;
}

// LCMapStringW rather than towupper: only it uppercases Vietnamese letters.
std::wstring toUpper(const std::wstring& s) {
    int needed = LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, s.c_str(), (int)s.size(), nullptr, 0);
    if (needed <= 0) return s;
    std::wstring out(needed + 1, L'\0');
    int written = LCMapStringW(LOCALE_INVARIANT, LCMAP_UPPERCASE, s.c_str(), (int)s.size(), &out[0], needed + 1);
    if (written <= 0) return s;
    out.resize(written);
    return out;
}

} // namespace

std::vector<Entry> parse(const std::wstring& text) {
    std::vector<Entry> entries;
    std::wistringstream stream(text);
    std::wstring line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line[0] == L'#') continue;

        size_t equalsPos = line.find(L'=');
        if (equalsPos == std::wstring::npos) continue;

        std::wstring trigger = trim(line.substr(0, equalsPos));
        if (trigger.empty() || trigger.size() > 31) continue;
        if (!std::all_of(trigger.begin(), trigger.end(), [](wchar_t c) { return std::iswalnum(c) != 0; })) continue;

        bool duplicate = false;
        for (const auto& entry : entries) {
            if (equalsNoCase(entry.trigger, trigger)) { duplicate = true; break; }
        }
        if (duplicate) continue;

        entries.push_back({ trigger, decodeEscapes(trim(line.substr(equalsPos + 1))) });
    }
    return entries;
}

bool find(const std::vector<Entry>& entries, const std::wstring& keys, std::wstring& expansion) {
    for (const auto& entry : entries) {
        if (!equalsNoCase(entry.trigger, keys)) continue;
        expansion = entry.expansion;

        bool hasAlpha = false;
        bool allUpper = true;
        wchar_t firstAlpha = 0;
        for (wchar_t c : keys) {
            if (!std::iswalpha(c)) continue;
            if (!firstAlpha) firstAlpha = c;
            hasAlpha = true;
            if (!std::iswupper(c)) allUpper = false;
        }

        if (hasAlpha && allUpper) {
            expansion = toUpper(expansion);
        } else if (hasAlpha && std::iswupper(firstAlpha)) {
            for (wchar_t& c : expansion) {
                if (std::iswalpha(c)) { c = (wchar_t)std::towupper(c); break; }
            }
        }
        return true;
    }
    return false;
}

const std::vector<Entry>& entries() {
    static std::vector<Entry> cache;
    static std::wstring cachedPath;
    static FILETIME cachedTime = {};
    static DWORD cachedSizeHigh = 0, cachedSizeLow = 0;
    static bool initialized = false;

    std::wstring path = listPath();
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &info)) return cache;

    if (initialized && cachedPath == path &&
        CompareFileTime(&cachedTime, &info.ftLastWriteTime) == 0 &&
        cachedSizeHigh == info.nFileSizeHigh && cachedSizeLow == info.nFileSizeLow) {
        return cache;
    }

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return cache;

    std::string bytes;
    char buffer[4096];
    for (;;) {
        DWORD bytesRead = 0;
        if (!ReadFile(hFile, buffer, sizeof(buffer), &bytesRead, nullptr) || bytesRead == 0) break;
        bytes.append(buffer, bytesRead);
    }
    CloseHandle(hFile);

    cachedPath = path;
    cachedTime = info.ftLastWriteTime;
    cachedSizeHigh = info.nFileSizeHigh;
    cachedSizeLow = info.nFileSizeLow;
    initialized = true;

    if (bytes.empty()) { cache.clear(); return cache; }

    UINT codepage = CP_UTF8;
    int wideCharCount = MultiByteToWideChar(codepage, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), nullptr, 0);
    if (wideCharCount == 0) {
        codepage = CP_ACP;
        wideCharCount = MultiByteToWideChar(codepage, 0, bytes.data(), (int)bytes.size(), nullptr, 0);
    }
    std::wstring wideText(wideCharCount, L'\0');
    if (wideCharCount > 0) MultiByteToWideChar(codepage, 0, bytes.data(), (int)bytes.size(), &wideText[0], wideCharCount);
    if (!wideText.empty() && wideText[0] == 0xFEFF) wideText.erase(0, 1);

    cache = parse(wideText);
    return cache;
}

bool enabled() {
    return vikey::prefs::getInt(L"MacroEnabled", 1) != 0;
}

std::wstring listPath() {
    std::wstring path = vikey::prefs::appDataDir() + L"\\macros.txt";
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) return path;

    std::wstring sample =
        L"# Gõ tắt của Vikey\n"
        L"# Mỗi dòng: viết tắt=thay thế\n"
        L"# Ví dụ:\n"
        L"# vn=Việt Nam\n"
        L"# ko=không\n";
    int byteCount = WideCharToMultiByte(CP_UTF8, 0, sample.c_str(), (int)sample.size(), nullptr, 0, nullptr, nullptr);
    std::string utf8(byteCount > 0 ? byteCount : 0, '\0');
    if (byteCount > 0) WideCharToMultiByte(CP_UTF8, 0, sample.c_str(), (int)sample.size(), &utf8[0], byteCount, nullptr, nullptr);

    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written = 0;
        WriteFile(hFile, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
        CloseHandle(hFile);
    }
    return path;
}

}}
