#include "win32_services.h"
#include <algorithm>
#include <cstring>

namespace snippet {
namespace {
constexpr const wchar_t* kInputError=L"文字入力に失敗しました (INPUT-SEND-001)。";
std::wstring ClipboardText(const std::wstring& text){
 std::wstring out;out.reserve(text.size());
 for(size_t i=0;i<text.size();++i){
  if(text[i]==L'\n'&&(i==0||text[i-1]!=L'\r'))out+=L"\r\n";
  else out+=text[i];
 }
 return out;
}
}

std::vector<INPUT> BuildPasteEvents(){
 std::vector<INPUT> events(4);
 for(auto& event:events){event.type=INPUT_KEYBOARD;event.ki.dwExtraInfo=kInputMarker;}
 events[0].ki.wVk=VK_CONTROL;
 events[1].ki.wVk=L'V';
 events[2].ki.wVk=L'V';events[2].ki.dwFlags=KEYEVENTF_KEYUP;
 events[3].ki.wVk=VK_CONTROL;events[3].ki.dwFlags=KEYEVENTF_KEYUP;
 return events;
}

std::wstring EmitText(const std::wstring& text){
 if(text.empty())return L"";
 if(!OpenClipboard(nullptr))return kInputError;
 EmptyClipboard();
 const std::wstring clipboardText=ClipboardText(text);
 const size_t bytes=(clipboardText.size()+1)*sizeof(wchar_t);
 HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);
 if(!memory){CloseClipboard();return kInputError;}
 void* data=GlobalLock(memory);
 if(!data){GlobalFree(memory);CloseClipboard();return kInputError;}
 memcpy(data,clipboardText.c_str(),bytes);
 GlobalUnlock(memory);
 if(!SetClipboardData(CF_UNICODETEXT,memory)){GlobalFree(memory);CloseClipboard();return kInputError;}
 CloseClipboard();
 auto events=BuildPasteEvents();
 const UINT sent=SendInput(static_cast<UINT>(events.size()),events.data(),sizeof(INPUT));
 return sent==events.size()?L"":kInputError;
}

SingleInstance::~SingleInstance(){if(handle_)CloseHandle(handle_);}
bool SingleInstance::Acquire(){if(handle_)return true;handle_=CreateMutexW(nullptr,FALSE,name_.c_str());if(!handle_)return false;if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(handle_);handle_=nullptr;return false;}return true;}
std::wstring StartupManager::Read() const {HKEY key{};if(RegOpenKeyExW(root_,key_.c_str(),0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return L"";DWORD type{},bytes{};if(RegQueryValueExW(key,value_.c_str(),nullptr,&type,nullptr,&bytes)!=ERROR_SUCCESS||type!=REG_SZ){RegCloseKey(key);return L"";}std::wstring out(bytes/sizeof(wchar_t),L'\0');if(RegQueryValueExW(key,value_.c_str(),nullptr,nullptr,reinterpret_cast<BYTE*>(out.data()),&bytes)!=ERROR_SUCCESS)out.clear();RegCloseKey(key);while(!out.empty()&&out.back()==L'\0')out.pop_back();return out;}
std::wstring StartupManager::Enable(const std::wstring& executable) const {HKEY key{};if(RegCreateKeyExW(root_,key_.c_str(),0,nullptr,0,KEY_SET_VALUE|KEY_QUERY_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS)return L"スタートアップを開けません (STARTUP-REG-001)。";const std::wstring command=L"\""+executable+L"\" --background";const auto status=RegSetValueExW(key,value_.c_str(),0,REG_SZ,reinterpret_cast<const BYTE*>(command.c_str()),static_cast<DWORD>((command.size()+1)*sizeof(wchar_t)));RegCloseKey(key);if(status!=ERROR_SUCCESS||Read()!=command)return L"スタートアップを更新できません (STARTUP-REG-001)。";return L"";}
std::wstring StartupManager::Disable(const std::wstring& ownedExecutable) const {if(!ownedExecutable.empty()&&Read()!=L"\""+ownedExecutable+L"\" --background")return L"";HKEY key{};if(RegOpenKeyExW(root_,key_.c_str(),0,KEY_SET_VALUE,&key)!=ERROR_SUCCESS)return L"";const auto status=RegDeleteValueW(key,value_.c_str());RegCloseKey(key);return status==ERROR_SUCCESS||status==ERROR_FILE_NOT_FOUND?L"":L"スタートアップを解除できません (STARTUP-REG-001)。";}
HotkeyRegistry::~HotkeyRegistry(){Clear();}
void HotkeyRegistry::Clear(){for(const auto& [id,_]:active_)UnregisterHotKey(window_,id);active_.clear();}
std::wstring HotkeyRegistry::Apply(const AppSettings& settings){Clear();int id=1;std::wstring error;for(const auto& item:settings.snippets){if(!item.enabled)continue;UINT modifiers=MOD_NOREPEAT;const auto mask=ModifierMask(item.hotkey);if(mask&1)modifiers|=MOD_CONTROL;if(mask&2)modifiers|=MOD_ALT;if(mask&4)modifiers|=MOD_SHIFT;if(mask&8)modifiers|=MOD_WIN;if(!RegisterHotKey(window_,id,modifiers,item.hotkey.virtualKey)){if(error.empty())error=FormatHotkey(item.hotkey)+L" は登録できません (HOTKEY-REG-001)。";++id;continue;}active_.emplace(id++,item);}return error;}
const Snippet* HotkeyRegistry::Resolve(int id) const {const auto it=active_.find(id);return it==active_.end()?nullptr:&it->second;}
bool HotkeyRegistry::IsRegistered(const std::wstring& snippetId) const {return std::any_of(active_.begin(),active_.end(),[&](const auto& item){return item.second.id==snippetId;});}
}
