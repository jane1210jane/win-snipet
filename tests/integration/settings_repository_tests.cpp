#include <Windows.h>
#include <filesystem>
#include "../../src/infrastructure/settings_repository.h"
#include "../test_runner.h"
using namespace snippet;

namespace { std::filesystem::path Temp(const wchar_t* name) { auto p=std::filesystem::temp_directory_path()/name; std::filesystem::remove_all(p); std::filesystem::create_directories(p); return p/L"settings.xml"; } }

TEST(FR06_settings_round_trip_preserves_unicode_xml_characters_and_newlines) {
    const auto path=Temp(L"SnippetToolTests-roundtrip"); XmlSettingsRepository repository(path);
    AppSettings source; source.startupEnabled=false; source.firstRunCompleted=true;
    source.snippets.push_back({L"{01234567-89AB-CDEF-0123-456789ABCDEF}",L"挨拶 & <確認>",L"こんにちは\n次の行",{{Modifier::Ctrl,Modifier::Alt},'H'},true});
    REQUIRE(repository.Save(source).empty()); auto loaded=repository.Load(); REQUIRE(loaded.ok());
    REQUIRE(loaded.settings->snippets[0].name==source.snippets[0].name); REQUIRE(loaded.settings->snippets[0].body==source.snippets[0].body);
}

TEST(FR06_corrupt_xml_is_reported_and_not_overwritten) {
    const auto path=Temp(L"SnippetToolTests-corrupt"); { HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr); const char data[]="<broken"; DWORD n{}; WriteFile(file,data,sizeof(data)-1,&n,nullptr); CloseHandle(file); }
    XmlSettingsRepository repository(path); auto loaded=repository.Load(); REQUIRE(!loaded.ok()); REQUIRE(!loaded.error.empty()); REQUIRE(std::filesystem::file_size(path)==7);
}

TEST(FR06_second_save_creates_backup_and_replaces_atomically) {
    const auto path=Temp(L"SnippetToolTests-atomic"); XmlSettingsRepository repository(path); AppSettings settings;
    REQUIRE(repository.Save(settings).empty()); settings.firstRunCompleted=true; REQUIRE(repository.Save(settings).empty());
    REQUIRE(std::filesystem::exists(path.wstring()+L".bak")); REQUIRE(repository.Load().settings->firstRunCompleted);
}
TEST(FR06_unknown_schema_version_is_rejected_without_modification) {
 const auto path=Temp(L"SnippetToolTests-schema");const char data[]="<snippetTool schemaVersion=\"99\"/>";HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);DWORD n{};WriteFile(file,data,sizeof(data)-1,&n,nullptr);CloseHandle(file);
 XmlSettingsRepository repository(path);const auto before=std::filesystem::file_size(path);const auto loaded=repository.Load();REQUIRE(!loaded.ok());REQUIRE(std::filesystem::file_size(path)==before);
}
