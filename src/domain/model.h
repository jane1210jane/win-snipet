#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace snippet {
enum class Modifier : std::uint8_t { Ctrl = 1, Alt = 2, Shift = 4, Win = 8 };
struct Hotkey { std::vector<Modifier> modifiers; unsigned virtualKey{}; };
struct Snippet { std::wstring id, name, body; Hotkey hotkey; bool enabled{true}; };
struct AppSettings { unsigned schemaVersion{1}; bool startupEnabled{true}; bool firstRunCompleted{false}; std::vector<Snippet> snippets; };
struct ValidationResult {
    std::optional<Snippet> value;
    std::vector<std::wstring> errors;
    [[nodiscard]] bool ok() const noexcept { return value.has_value() && errors.empty(); }
};
[[nodiscard]] ValidationResult ValidateSnippet(const Snippet& snippet);
[[nodiscard]] std::vector<std::wstring> ValidateHotkey(const Hotkey& hotkey);
[[nodiscard]] std::vector<std::wstring> ValidateSettings(const AppSettings& settings);
[[nodiscard]] std::wstring FormatHotkey(const Hotkey& hotkey);
[[nodiscard]] unsigned ModifierMask(const Hotkey& hotkey) noexcept;
}
