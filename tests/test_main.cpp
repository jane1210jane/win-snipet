#include "test_runner.h"
#include <Windows.h>
#include <filesystem>

int main() {
    int failures = 0, skipped = 0;
    for (const auto& test : Tests()) {
        try { test.run(); std::wcout << L"PASS " << test.name << L'\n'; }
        catch (const SkipException& reason) { ++skipped; std::cerr << "SKIP " << test.name << ": " << reason.what() << '\n'; }
        catch (const std::exception& error) { ++failures; std::cerr << "FAIL " << test.name << ": " << error.what() << '\n'; }
    }
    RegDeleteTreeW(HKEY_CURRENT_USER,L"Software\\SnippetToolTests");
    const wchar_t* temporaryDirectories[]={L"SnippetToolTests-roundtrip",L"SnippetToolTests-corrupt",L"SnippetToolTests-atomic",L"SnippetToolTests-schema",L"SnippetToolTests-logger"};
    for(const auto* name:temporaryDirectories){std::error_code error;std::filesystem::remove_all(std::filesystem::temp_directory_path()/name,error);}
    std::cout << Tests().size() - failures - skipped << "/" << Tests().size() << " tests passed, " << skipped << " skipped\n";
    return failures == 0 ? 0 : 1;
}
