#pragma once
#include <Windows.h>
#include <map>
#include <string>
#include <vector>
#include "../domain/model.h"
#include "../application/snippet_service.h"
namespace snippet {
inline constexpr ULONG_PTR kInputMarker=0x534E4950544F4F4Cull;
[[nodiscard]] std::vector<INPUT> BuildPasteEvents();
[[nodiscard]] std::wstring EmitText(const std::wstring& text);

class SingleInstance {
public: explicit SingleInstance(std::wstring name):name_(std::move(name)){} ~SingleInstance(); SingleInstance(const SingleInstance&)=delete; SingleInstance& operator=(const SingleInstance&)=delete;
 [[nodiscard]] bool Acquire();
private: std::wstring name_; HANDLE handle_{};
};

class StartupManager {
public: StartupManager(HKEY root=HKEY_CURRENT_USER,std::wstring key=L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",std::wstring value=L"SnippetTool"):root_(root),key_(std::move(key)),value_(std::move(value)){}
 [[nodiscard]] std::wstring Enable(const std::wstring& executable) const; [[nodiscard]] std::wstring Disable(const std::wstring& ownedExecutable=L"") const; [[nodiscard]] std::wstring Read() const;
private:HKEY root_;std::wstring key_,value_;
};

class HotkeyRegistry final:public IHotkeySet {
public: explicit HotkeyRegistry(HWND window):window_(window){} ~HotkeyRegistry(); [[nodiscard]] std::wstring Apply(const AppSettings& settings) override; void Clear(); [[nodiscard]] const Snippet* Resolve(int id) const;
 [[nodiscard]] bool IsRegistered(const std::wstring& snippetId) const;
private:HWND window_;std::map<int,Snippet> active_;
};
}
