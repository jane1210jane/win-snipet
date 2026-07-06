#pragma once
#include "../domain/model.h"
namespace snippet {
struct ISettingsStore {virtual ~ISettingsStore()=default;[[nodiscard]] virtual std::wstring Save(const AppSettings&)=0;};
struct IHotkeySet {virtual ~IHotkeySet()=default;[[nodiscard]] virtual std::wstring Apply(const AppSettings&)=0;};
class SnippetService {
public:SnippetService(AppSettings& settings,ISettingsStore& store,IHotkeySet& keys):settings_(settings),store_(store),keys_(keys){}
 [[nodiscard]] std::wstring Add(const Snippet&);[[nodiscard]] std::wstring Update(const std::wstring& id,const Snippet&);[[nodiscard]] std::wstring Remove(const std::wstring& id);
private:[[nodiscard]] std::wstring Commit(AppSettings next);AppSettings& settings_;ISettingsStore& store_;IHotkeySet& keys_;
};
}
