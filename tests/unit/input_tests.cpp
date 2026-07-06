#include <Windows.h>
#include "../../src/infrastructure/win32_services.h"
#include "../test_runner.h"
using namespace snippet;

TEST(FR04_paste_input_builds_ctrl_v_sequence) {
    const auto events=BuildPasteEvents();
    REQUIRE(events.size()==4);
    REQUIRE(events[0].ki.wVk==VK_CONTROL && events[0].ki.dwFlags==0);
    REQUIRE(events[1].ki.wVk==L'V' && events[1].ki.dwFlags==0);
    REQUIRE(events[2].ki.wVk==L'V' && events[2].ki.dwFlags==KEYEVENTF_KEYUP);
    REQUIRE(events[3].ki.wVk==VK_CONTROL && events[3].ki.dwFlags==KEYEVENTF_KEYUP);
}

TEST(NFR02_input_events_carry_marker) {
    for(const auto& event:BuildPasteEvents()) REQUIRE(event.ki.dwExtraInfo==kInputMarker);
}
