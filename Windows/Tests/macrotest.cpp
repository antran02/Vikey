#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include "Macro.h"

int wmain() {
    SetConsoleOutputCP(CP_UTF8);

    int passed = 0;
    int total = 0;
    auto check = [&](bool ok, const wchar_t* name) {
        ++total;
        if (ok) { ++passed; std::wcout << L"ok  " << name << std::endl; }
        else std::wcout << L"FAIL " << name << std::endl;
    };

    const std::wstring text =
        L"# Comment\n"
        L"trigger1=one\n"
        L"  trigger2  =  two  \n"
        L"bad trigger=x\n"
        L"bad!trigger=x\n"
        L"trigger3=three\n"
        L"TRIGGER3=three-again\n"
        L"newline=line1\\nline2\n"
        L"tab=a\\tb\n"
        L"back=slash\\\\here\n"
        L"unknown=a\\qb\n"
        L"empty=\n";

    std::vector<vikey::macro::Entry> entries = vikey::macro::parse(text);

    check(entries.size() == 8, L"parse_count");
    check(entries.size() > 0 && entries[0].trigger == L"trigger1" && entries[0].expansion == L"one", L"parse_first");
    check(entries.size() > 1 && entries[1].trigger == L"trigger2" && entries[1].expansion == L"two", L"parse_trimmed");
    check(entries.size() > 2 && entries[2].trigger == L"trigger3" && entries[2].expansion == L"three", L"parse_duplicate_skipped");
    check(entries.size() > 6 && entries[6].trigger == L"unknown" && entries[6].expansion == L"a\\qb", L"parse_unknown_escape");

    std::wstring expansion;

    check(vikey::macro::find(entries, L"trigger1", expansion) && expansion == L"one", L"find_exact");
    check(vikey::macro::find(entries, L"trigger2", expansion) && expansion == L"two", L"find_trimmed");
    check(vikey::macro::find(entries, L"TRIGGER2", expansion) && expansion == L"TWO", L"find_all_caps");
    check(vikey::macro::find(entries, L"Trigger2", expansion) && expansion == L"Two", L"find_title_case");
    check(!vikey::macro::find(entries, L"nonexistent", expansion), L"find_no_match");
    check(vikey::macro::find(entries, L"newline", expansion) && expansion == L"line1\nline2", L"find_newline_escape");
    check(vikey::macro::find(entries, L"tab", expansion) && expansion == L"a\tb", L"find_tab_escape");
    check(vikey::macro::find(entries, L"back", expansion) && expansion == L"slash\\here", L"find_backslash_escape");
    check(vikey::macro::find(entries, L"unknown", expansion) && expansion == L"a\\qb", L"find_unknown_escape");
    check(vikey::macro::find(entries, L"empty", expansion) && expansion.empty(), L"find_empty_expansion");
    check(vikey::macro::find(entries, L"trigger3", expansion) && expansion == L"three", L"find_first_duplicate");

    std::wcout << passed << L"/" << total << L" passed" << std::endl;
    return total - passed;
}
