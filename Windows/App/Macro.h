#pragma once
#include <string>
#include <vector>

namespace vikey { namespace macro {

struct Entry { std::wstring trigger; std::wstring expansion; };

// Parse the macro file format (see below). Never throws.
std::vector<Entry> parse(const std::wstring& text);

// Case-insensitive lookup. On a hit, fills `expansion` (case-adapted) and returns true.
bool find(const std::vector<Entry>& entries, const std::wstring& keys, std::wstring& expansion);

// Macros from %LOCALAPPDATA%\Vikey\macros.txt, reloaded when the file's last-write time changes.
const std::vector<Entry>& entries();

// false when the "MacroEnabled" pref is 0 (default 1).
bool enabled();

// Full path of the editable list; creates the file with a commented sample when missing.
std::wstring listPath();

}}
