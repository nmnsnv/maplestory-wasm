// Tier 1 UI-logic tests: keyboard mapping (Plan 07).
// Keyboard is pure logic over GLFW key codes; it uses no GL/asset state. The
// GLFW_KEY_* constants come from the host test shim (Test/glfw_shim/).
#include <doctest/doctest.h>

#include "IO/Keyboard.h"
#include "IO/KeyAction.h"
#include "IO/KeyType.h"

#include <GLFW/glfw3.h>

using namespace jrc;

TEST_CASE("Keyboard reports the modifier key codes")
{
    Keyboard keyboard;
    CHECK(keyboard.shiftcode() == GLFW_KEY_LEFT_SHIFT);
    CHECK(keyboard.ctrlcode() == GLFW_KEY_LEFT_CONTROL);
}

TEST_CASE("Keyboard maps the default action keys")
{
    Keyboard keyboard;

    auto left = keyboard.get_mapping(GLFW_KEY_LEFT);
    CHECK(left.type == KeyType::ACTION);
    CHECK(left.action == KeyAction::LEFT);

    auto enter = keyboard.get_mapping(GLFW_KEY_ENTER);
    CHECK(enter.type == KeyType::ACTION);
    CHECK(enter.action == KeyAction::RETURN);

    // Unmapped keys fall back to the default (NONE) mapping.
    auto unmapped = keyboard.get_mapping(GLFW_KEY_F12);
    CHECK(unmapped.type == KeyType::NONE);
}

TEST_CASE("Keyboard ctrl-combos resolve to clipboard actions")
{
    Keyboard keyboard;
    CHECK(keyboard.get_ctrl_action(GLFW_KEY_C) == KeyAction::COPY);
    CHECK(keyboard.get_ctrl_action(GLFW_KEY_V) == KeyAction::PASTE);
    CHECK(keyboard.get_ctrl_action(GLFW_KEY_A) == KeyAction::NOACTION);
}

TEST_CASE("Keyboard assign/remove updates the maple key mapping")
{
    Keyboard keyboard;

    // key index 2 -> Keytable[2] == GLFW_KEY_1 (see Keyboard.cpp Keytable).
    keyboard.assign(2, KeyType::SKILL, 1138);

    auto mapped = keyboard.get_maple_mapping(2);
    CHECK(mapped.type == KeyType::SKILL);
    CHECK(mapped.action == 1138);

    // The same assignment is reachable through the GLFW keymap too.
    auto via_keymap = keyboard.get_mapping(GLFW_KEY_1);
    CHECK(via_keymap.type == KeyType::SKILL);
    CHECK(via_keymap.action == 1138);

    keyboard.remove(2);
    CHECK(keyboard.get_maple_mapping(2).type == KeyType::NONE);
}

TEST_CASE("Keyboard text mapping honors the shift modifier")
{
    Keyboard keyboard;

    auto lower_a = keyboard.get_text_mapping(GLFW_KEY_A, false);
    CHECK(lower_a.type == KeyType::LETTER);
    CHECK(lower_a.action == 'a');

    auto upper_a = keyboard.get_text_mapping(GLFW_KEY_A, true);
    CHECK(upper_a.type == KeyType::LETTER);
    CHECK(upper_a.action == 'A');

    auto digit_1 = keyboard.get_text_mapping(GLFW_KEY_1, false);
    CHECK(digit_1.action == '1');

    auto shift_1 = keyboard.get_text_mapping(GLFW_KEY_1, true);
    CHECK(shift_1.action == '!');

    auto space = keyboard.get_text_mapping(GLFW_KEY_SPACE, false);
    CHECK(space.type == KeyType::ACTION);
    CHECK(space.action == KeyAction::SPACE);
}
