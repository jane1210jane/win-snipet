#include <Windows.h>
#include "../../src/infrastructure/win32_services.h"
#include "../test_runner.h"
using namespace snippet;

TEST(FR07_named_mutex_allows_exactly_one_owner) {
    SingleInstance first(L"Local\\SnippetTool-Test-0A7E");
    SingleInstance second(L"Local\\SnippetTool-Test-0A7E");
    REQUIRE(first.Acquire());
    REQUIRE(!second.Acquire());
}

TEST(FR08_startup_manager_uses_quoted_path_and_only_removes_owned_value) {
    StartupManager manager(HKEY_CURRENT_USER,L"Software\\SnippetToolTests",L"Startup-Test");
    REQUIRE(manager.Disable().empty());
    REQUIRE(manager.Enable(L"C:\\Program Files\\Snippet Tool\\SnippetTool.exe").empty());
    REQUIRE(manager.Read()==L"\"C:\\Program Files\\Snippet Tool\\SnippetTool.exe\" --background");
    REQUIRE(manager.Disable(L"C:\\Program Files\\Snippet Tool\\SnippetTool.exe").empty());
    REQUIRE(manager.Read().empty());
}

TEST(FR08_startup_manager_preserves_a_value_it_does_not_own) {
    StartupManager manager(HKEY_CURRENT_USER,L"Software\\SnippetToolTests",L"Startup-Foreign-Test");
    REQUIRE(manager.Enable(L"C:\\foreign.exe").empty());
    REQUIRE(manager.Disable(L"C:\\SnippetTool.exe").empty());
    REQUIRE(!manager.Read().empty());
    REQUIRE(manager.Disable(L"C:\\foreign.exe").empty());
}
