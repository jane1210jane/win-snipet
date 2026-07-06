#include "snippet_service.h"
#include <algorithm>
namespace snippet {
std::wstring SnippetService::Commit(AppSettings next){if(const auto errors=ValidateSettings(next);!errors.empty())return errors.front();const auto previous=settings_;if(const auto e=keys_.Apply(next);!e.empty())return e;if(const auto e=store_.Save(next);!e.empty()){const auto ignored=keys_.Apply(previous);(void)ignored;return e;}settings_=std::move(next);return L"";}
std::wstring SnippetService::Add(const Snippet& input){const auto validated=ValidateSnippet(input);if(!validated.ok())return validated.errors.front();auto next=settings_;next.snippets.push_back(*validated.value);return Commit(std::move(next));}
std::wstring SnippetService::Update(const std::wstring& id,const Snippet& input){const auto validated=ValidateSnippet(input);if(!validated.ok())return validated.errors.front();auto next=settings_;const auto it=std::find_if(next.snippets.begin(),next.snippets.end(),[&](const Snippet& s){return s.id==id;});if(it==next.snippets.end())return L"スニペットが見つかりません。";*it=*validated.value;it->id=id;return Commit(std::move(next));}
std::wstring SnippetService::Remove(const std::wstring& id){auto next=settings_;const auto old=next.snippets.size();std::erase_if(next.snippets,[&](const Snippet& s){return s.id==id;});if(next.snippets.size()==old)return L"スニペットが見つかりません。";return Commit(std::move(next));}
}
