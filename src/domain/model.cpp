#include "model.h"
#include <algorithm>
#include <cwctype>
#include <set>
#include <sstream>

namespace snippet {
namespace {
constexpr unsigned kEscape=0x1B, kTab=0x09, kEnter=0x0D, kSpace=0x20, kBack=0x08, kDelete=0x2E;
constexpr unsigned kPrint=0x2C, kPause=0x13, kCaps=0x14, kNum=0x90, kScroll=0x91;
std::wstring Trim(std::wstring value) {
    const auto first=std::find_if_not(value.begin(),value.end(),iswspace);
    const auto last=std::find_if_not(value.rbegin(),value.rend(),iswspace).base();
    return first<last ? std::wstring(first,last) : L"";
}
std::wstring NormalizeNewlines(const std::wstring& input) {
    std::wstring output; output.reserve(input.size());
    for (size_t i=0;i<input.size();++i) { if(input[i]==L'\r'){ output.push_back(L'\n'); if(i+1<input.size()&&input[i+1]==L'\n')++i; } else output.push_back(input[i]); }
    return output;
}
bool IsGuid(const std::wstring& id) {
    if(id.size()!=38||id.front()!=L'{'||id.back()!=L'}') return false;
    for(size_t i=1;i<37;++i){ if(i==9||i==14||i==19||i==24){if(id[i]!=L'-')return false;} else if(!iswxdigit(id[i]))return false; }
    return true;
}
}
unsigned ModifierMask(const Hotkey& hotkey) noexcept { unsigned mask=0; for(const auto m:hotkey.modifiers) mask|=static_cast<unsigned>(m); return mask; }
std::vector<std::wstring> ValidateHotkey(const Hotkey& hotkey) {
    std::vector<std::wstring> errors;
    if(ModifierMask(hotkey)==0) errors.push_back(L"修飾キーを1つ以上指定してください。");
    const std::set<unsigned> forbidden{kEscape,kTab,kEnter,kSpace,kBack,kDelete,kPrint,kPause,kCaps,kNum,kScroll};
    const bool alphaNum=(hotkey.virtualKey>=L'0'&&hotkey.virtualKey<=L'9')||(hotkey.virtualKey>=L'A'&&hotkey.virtualKey<=L'Z');
    const bool function=hotkey.virtualKey>=0x70&&hotkey.virtualKey<=0x87;
    const bool navigation=hotkey.virtualKey>=0x21&&hotkey.virtualKey<=0x28;
    const bool oem=hotkey.virtualKey>=0xBA&&hotkey.virtualKey<=0xE2;
    if(forbidden.contains(hotkey.virtualKey)||!(alphaNum||function||navigation||oem)) errors.push_back(L"このキーは割り当てできません。");
    return errors;
}
ValidationResult ValidateSnippet(const Snippet& input) {
    ValidationResult result; Snippet value=input; value.name=Trim(value.name); value.body=NormalizeNewlines(value.body);
    if(!IsGuid(value.id)) result.errors.push_back(L"IDが不正です。");
    if(value.name.empty()||value.name.size()>100) result.errors.push_back(L"名称は1～100文字で入力してください。");
    if(value.body.empty()||value.body.size()>32768) result.errors.push_back(L"本文は1～32768文字で入力してください。");
    const auto keyErrors=ValidateHotkey(value.hotkey); result.errors.insert(result.errors.end(),keyErrors.begin(),keyErrors.end());
    if(result.errors.empty()) result.value=std::move(value); return result;
}
std::vector<std::wstring> ValidateSettings(const AppSettings& settings) {
    std::vector<std::wstring> errors; std::set<std::pair<unsigned,unsigned>> keys; std::set<std::wstring> ids;
    const auto pickerErrors=ValidateHotkey(settings.pickerHotkey);errors.insert(errors.end(),pickerErrors.begin(),pickerErrors.end());
    keys.emplace(ModifierMask(settings.pickerHotkey),settings.pickerHotkey.virtualKey);
    for(const auto& item:settings.snippets){ auto validated=ValidateSnippet(item); errors.insert(errors.end(),validated.errors.begin(),validated.errors.end());
        std::wstring id=item.id; std::transform(id.begin(),id.end(),id.begin(),towupper); if(!ids.insert(id).second) errors.push_back(L"IDが重複しています。");
        const unsigned key=item.hotkey.virtualKey; if(!keys.emplace(ModifierMask(item.hotkey),key).second) errors.push_back(L"ショートカットが重複しています。"); }
    return errors;
}
std::wstring FormatHotkey(const Hotkey& hotkey) {
    std::wstring result; const unsigned mask=ModifierMask(hotkey);
    if(mask&1)result+=L"Ctrl+"; if(mask&2)result+=L"Alt+"; if(mask&4)result+=L"Shift+"; if(mask&8)result+=L"Win+";
    unsigned key=hotkey.virtualKey;
    if(key>=0x70&&key<=0x87) result+=L"F"+std::to_wstring(key-0x6F); else { if(key>=L'a'&&key<=L'z')key-=32; if(key>=32&&key<127) result.push_back(static_cast<wchar_t>(key)); else result+=L"VK("+std::to_wstring(key)+L")"; }
    return result;
}
std::wstring FormatPickerItem(const Snippet& snippet) {
    std::wstring preview=snippet.body.substr(0,15);
    std::replace(preview.begin(),preview.end(),L'\n',L' ');std::replace(preview.begin(),preview.end(),L'\r',L' ');
    if(snippet.body.size()>15)preview+=L"…";
    return snippet.name+L" — "+preview;
}
}
