#include "../../src/application/snippet_service.h"
#include "../test_runner.h"
using namespace snippet;
namespace {
struct Repo final:ISettingsStore { bool fail{};int saves{};std::wstring Save(const AppSettings&) override{++saves;return fail?L"save failed":L"";} };
struct Keys final:IHotkeySet { bool fail{};int applies{};AppSettings last;std::wstring Apply(const AppSettings& s) override{++applies;last=s;return fail?L"key failed":L"";} };
Snippet Valid(const wchar_t* id,const wchar_t* name){return {id,name,L"body",{{Modifier::Ctrl},'H'},true};}
}
TEST(FR02_service_add_validates_registers_and_persists) {
 Repo repo;Keys keys;AppSettings settings;SnippetService service(settings,repo,keys);
 const auto result=service.Add(Valid(L"{01234567-89AB-CDEF-0123-456789ABCDEF}",L"one"));
 REQUIRE(result.empty());REQUIRE(settings.snippets.size()==1);REQUIRE(repo.saves==1);REQUIRE(keys.applies==1);
}
TEST(FR03_service_does_not_persist_when_hotkey_registration_fails) {
 Repo repo;Keys keys;keys.fail=true;AppSettings settings;SnippetService service(settings,repo,keys);
 REQUIRE(!service.Add(Valid(L"{01234567-89AB-CDEF-0123-456789ABCDEF}",L"one")).empty());
 REQUIRE(settings.snippets.empty());REQUIRE(repo.saves==0);
}
TEST(FR06_service_rolls_hotkeys_back_when_persistence_fails) {
 Repo repo;repo.fail=true;Keys keys;AppSettings settings;SnippetService service(settings,repo,keys);
 REQUIRE(!service.Add(Valid(L"{01234567-89AB-CDEF-0123-456789ABCDEF}",L"one")).empty());
 REQUIRE(settings.snippets.empty());REQUIRE(keys.applies==2);REQUIRE(keys.last.snippets.empty());
}
TEST(FR02_service_updates_and_removes_by_immutable_id) {
 Repo repo;Keys keys;AppSettings settings;settings.snippets.push_back(Valid(L"{01234567-89AB-CDEF-0123-456789ABCDEF}",L"one"));SnippetService service(settings,repo,keys);
 auto changed=Valid(L"{11234567-89AB-CDEF-0123-456789ABCDEF}",L"changed");
 REQUIRE(service.Update(settings.snippets[0].id,changed).empty());REQUIRE(settings.snippets[0].name==L"changed");REQUIRE(settings.snippets[0].id==L"{01234567-89AB-CDEF-0123-456789ABCDEF}");
 REQUIRE(service.Remove(settings.snippets[0].id).empty());REQUIRE(settings.snippets.empty());
}
