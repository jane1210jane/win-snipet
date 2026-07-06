#include <Windows.h>
#include "../../src/domain/model.h"
#include "../test_runner.h"

using namespace snippet;

TEST(FR02_name_is_trimmed_and_body_newlines_are_normalized) {
    Snippet draft{L"{01234567-89AB-CDEF-0123-456789ABCDEF}", L"  挨拶  ", L"a\r\nb\rc", {{Modifier::Ctrl}, 'H'}, true};
    const auto result = ValidateSnippet(draft);
    REQUIRE(result.ok());
    REQUIRE(result.value->name == L"挨拶");
    REQUIRE(result.value->body == L"a\nb\nc");
}

TEST(FR02_empty_and_oversized_fields_are_rejected) {
    Snippet draft{L"bad", L"   ", std::wstring(32769, L'x'), {{Modifier::Ctrl}, 'H'}, true};
    const auto result = ValidateSnippet(draft);
    REQUIRE(!result.ok());
    REQUIRE(result.errors.size() >= 3);
}

TEST(FR03_hotkey_requires_modifier_and_permitted_key) {
    REQUIRE(!ValidateHotkey({{}, 'A'}).empty());
    REQUIRE(!ValidateHotkey({{Modifier::Ctrl}, VK_ESCAPE}).empty());
    REQUIRE(!ValidateHotkey({{Modifier::Ctrl}, 'a'}).empty());
    REQUIRE(ValidateHotkey({{Modifier::Ctrl, Modifier::Alt}, 'A'}).empty());
}

TEST(FR03_duplicate_hotkeys_are_case_insensitive_and_include_disabled_items) {
    AppSettings settings;
    settings.snippets.push_back({L"{01234567-89AB-CDEF-0123-456789ABCDEF}", L"one", L"1", {{Modifier::Ctrl}, 'A'}, true});
    settings.snippets.push_back({L"{11234567-89AB-CDEF-0123-456789ABCDEF}", L"two", L"2", {{Modifier::Ctrl}, 'A'}, false});
    REQUIRE(ValidateSettings(settings).size() == 1);
}

TEST(FR03_function_key_does_not_collide_with_letter_key) {
    AppSettings settings;
    settings.snippets.push_back({L"{01234567-89AB-CDEF-0123-456789ABCDEF}", L"f2", L"1", {{Modifier::Ctrl}, VK_F2}, true});
    settings.snippets.push_back({L"{11234567-89AB-CDEF-0123-456789ABCDEF}", L"q", L"2", {{Modifier::Ctrl}, 'Q'}, true});
    REQUIRE(ValidateSettings(settings).empty());
}

TEST(FR03_hotkey_display_has_stable_modifier_order) {
    const auto formatted = FormatHotkey({{Modifier::Win, Modifier::Shift, Modifier::Alt, Modifier::Ctrl}, VK_F2});
    if (formatted != L"Ctrl+Alt+Shift+Win+F2") { std::wcerr << L"Actual: " << formatted << L'\n'; }
    REQUIRE(formatted == L"Ctrl+Alt+Shift+Win+F2");
}
