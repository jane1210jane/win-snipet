#include <Windows.h>
#include "../../src/infrastructure/win32_services.h"
#include "../test_runner.h"
using namespace snippet;
TEST(FR04_SendInput_enters_unicode_symbols_and_newline_in_edit_control) {
 HWND host=CreateWindowExW(0,L"STATIC",L"SnippetTool input target",WS_OVERLAPPEDWINDOW,0,0,400,200,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 REQUIRE(host!=nullptr);HWND edit=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|ES_MULTILINE,0,0,380,160,host,nullptr,GetModuleHandleW(nullptr),nullptr);
 REQUIRE(edit!=nullptr);ShowWindow(host,SW_SHOW);SetForegroundWindow(host);SetFocus(edit);
 if(GetForegroundWindow()!=host){DestroyWindow(host);SKIP("interactive foreground desktop is unavailable");}
 const auto emitError=EmitText(L"日本語 !\nnext");if(!emitError.empty()){DestroyWindow(host);SKIP("SendInput is blocked in this desktop session");}
 const DWORD deadline=GetTickCount()+1000;MSG message{};while(GetTickCount()<deadline){while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}if(GetWindowTextLengthW(edit)>=11)break;Sleep(10);}
 const int length=GetWindowTextLengthW(edit);std::wstring actual(static_cast<size_t>(length)+1,L'\0');GetWindowTextW(edit,actual.data(),length+1);actual.resize(length);DestroyWindow(host);
 REQUIRE(actual==L"日本語 !\r\nnext");
}
