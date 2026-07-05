//////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
// Copyright © 2015-2016 Daniel Allendorf                                   //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////
#include "Configuration.h"
#ifdef MS_PLATFORM_WASM
#include <emscripten.h>
#endif
#include "Constants.h"
#include "Error.h"
#include "Timer.h"

#include "Audio/Audio.h"
#include "Character/Char.h"
#include "Gameplay/Combat/DamageNumber.h"
#include "Gameplay/Stage.h"
#include "Gameplay/MapleMap/Npc.h"
#include "IO/UI.h"
#include "IO/UITypes/UICharSelect.h"
#include "IO/UITypes/UINpcTalk.h"
#include "IO/Window.h"
#include "Net/Packets/GameplayPackets.h"
#include "Net/Packets/MessagingPackets.h"
#include "Net/Session.h"
#include "Util/NxFiles.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>


namespace jrc
{
    Error init()
    {
#ifdef MS_PLATFORM_WASM
        auto loadConfigString = [](const char* key, auto& setting) {
            char* val = (char*)EM_ASM_INT({
                var k = UTF8ToString($0);
                if (typeof Module !== 'undefined' && Module.LazyFS && Module.LazyFS[k] !== undefined && Module.LazyFS[k] !== null) {
                    var str = Module.LazyFS[k].toString();
                    var lengthBytes = lengthBytesUTF8(str) + 1;
                    var stringOnWasmHeap = _malloc(lengthBytes);
                    stringToUTF8(str, stringOnWasmHeap, lengthBytes);
                    return stringOnWasmHeap;
                }
                return 0;
            }, key);
            
            if (val) {
                setting.save(std::string(val));
                free(val);
            }
        };

        loadConfigString("MapleStoryServerIp", Setting<MapleStoryServerIp>::get());
        loadConfigString("MapleStoryServerPort", Setting<MapleStoryServerPort>::get());
        loadConfigString("ProxyIP", Setting<ProxyIP>::get());
        loadConfigString("ProxyPort", Setting<ProxyPort>::get());
        loadConfigString("AssetsServerProtocol", Setting<AssetsServerProtocol>::get());
        loadConfigString("AutoLoginAccount", Setting<AutoLoginAccount>::get());
        loadConfigString("AutoLoginPassword", Setting<AutoLoginPassword>::get());
        loadConfigString("AutoLoginCharacter", Setting<AutoLoginCharacter>::get());
#endif

        if (Error error = Session::get().init())
        {
            return error;
        }

        if (Error error = NxFiles::init())
        {
            return error;
        }

        if (Error error = Window::get().init())
        {
            return error;
        }

        if (Error error = Sound::init())
        {
            return error;
        }

        if (Error error = Music::init())
        {
            return error;
        }

        Char::init();
        DamageNumber::init();
        MapPortals::init();
        Stage::get().init();
        UI::get().init();

        return Error::NONE;
    }

    void update()
    {
        Window::get().check_events();
        Window::get().update();
        Stage::get().update();
        UI::get().update();
        Session::get().read();
    }

    void draw(float alpha)
    {
        Window::get().begin();
        Stage::get().draw(alpha);
        UI::get().draw(alpha);
        Window::get().end();
    }

    bool running()
    {
        return Session::get().is_connected()
            && UI::get().not_quitted()
            && Window::get().not_closed();
    }

#ifdef MS_PLATFORM_WASM
    static int64_t timestep = Constants::TIMESTEP * 1000;
    static int64_t accumulator = timestep;
    static int64_t period = 0;
    static int32_t samples = 0;

    void main_tick()
    {
        if (!running())
        {
            emscripten_cancel_main_loop();
            Sound::close();
            return;
        }

        int64_t elapsed = Timer::get().stop();

        for (accumulator += elapsed; accumulator >= timestep; accumulator -= timestep)
            update();

        float alpha = static_cast<float>(accumulator) / timestep;
        draw(alpha);

        bool show_fps = true; // Hardcoded or from config
        if (show_fps)
        {
            if (samples < 100)
            {
                period += elapsed;
                samples++;
            }
            else if (period)
            {
                period = 0;
                samples = 0;
            }
        }
    }
#endif

    void loop()
    {
        Timer::get().start();

#ifdef MS_PLATFORM_WASM
        accumulator = timestep;
        emscripten_set_main_loop(main_tick, 0, 1);
#else
        int64_t timestep    = Constants::TIMESTEP * 1000;
        int64_t accumulator = timestep;

        int64_t period  = 0;
        int32_t samples = 0;

        while (running())
        {
            int64_t elapsed = Timer::get().stop();

            // Update game with constant timestep as many times as possible.
            for (accumulator += elapsed; accumulator >= timestep; accumulator -= timestep)
            {
                update();
            }

            // Draw the game. Interpolate to account for remaining time.
            float alpha = static_cast<float>(accumulator) / timestep;
            draw(alpha);

            if (samples < 100)
            {
                period += elapsed;
                samples++;
            }
            else if (period)
            {
                //int64_t fps = (samples * 1000000) / period;
                //std::cout << "FPS: " << fps << std::endl;

                period  = 0;
                samples = 0;
            }
        }

        Sound::close();
#endif
    }

    void start()
    {
        // Initialize and check for errors.
        if (Error error = init())
        {
            const char* message   = error.get_message();
            const char* args      = error.get_args();
            const bool  can_retry = error.can_retry();

            std::cout << "Error: " << message << args << std::endl;

            std::string command;
            std::cin >> command;

            if (can_retry && command == "retry")
            {
                start();
            }
        }
        else
        {
            loop();
        }
    }
}

#ifdef MS_PLATFORM_WASM
namespace
{
    const jrc::UINpcTalk* active_npc_dialogue()
    {
        if (auto npctalk = jrc::UI::get().get_element<jrc::UINpcTalk>())
        {
            return npctalk->is_active() ? npctalk.get() : nullptr;
        }

        return nullptr;
    }

    jrc::UINpcTalk* active_npc_dialogue_mutable()
    {
        if (auto npctalk = jrc::UI::get().get_element<jrc::UINpcTalk>())
        {
            return npctalk->is_active() ? npctalk.get() : nullptr;
        }

        return nullptr;
    }

    const jrc::Npc* active_npc_at(int32_t index)
    {
        if (index < 0)
        {
            return nullptr;
        }

        const jrc::MapObjects* npcs = jrc::Stage::get().get_npcs().get_npcs();
        int32_t current = 0;
        for (const auto& entry : *npcs)
        {
            const jrc::Npc* npc = static_cast<const jrc::Npc*>(entry.second.get());
            if (!npc || !npc->is_active())
            {
                continue;
            }

            if (current == index)
            {
                return npc;
            }
            ++current;
        }

        return nullptr;
    }

    int32_t active_npc_count()
    {
        const jrc::MapObjects* npcs = jrc::Stage::get().get_npcs().get_npcs();
        int32_t count = 0;
        for (const auto& entry : *npcs)
        {
            const jrc::Npc* npc = static_cast<const jrc::Npc*>(entry.second.get());
            if (npc && npc->is_active())
            {
                ++count;
            }
        }
        return count;
    }

    void copy_string_to_buffer(const std::string& source, char* out, int32_t out_size)
    {
        if (!out || out_size <= 0)
        {
            return;
        }

        const size_t capacity = static_cast<size_t>(out_size);
        const size_t count = std::min(source.size(), capacity - 1);
        std::memcpy(out, source.data(), count);
        out[count] = '\0';
    }
}

extern "C" {
// JS-callable logout: closes the game server connection, reconnects to the
// login server, and shows the login screen. Call via:
//   Module._maple_logout()
// or
//   ccall('maple_logout')
EMSCRIPTEN_KEEPALIVE
void maple_logout()
{
    jrc::UI::get().set_skip_auto_login();
    jrc::Session::get().logout();
    jrc::UI::get().change_state(jrc::UI::LOGIN);
}

// JS-callable re-login: same as maple_logout but does NOT suppress auto-login,
// so the client immediately re-enters credentials and advances to char select.
// Pair with maple_set_auto_login_character to switch characters without a page reload.
EMSCRIPTEN_KEEPALIVE
void maple_relogin()
{
    jrc::Session::get().logout();
    jrc::UI::get().change_state(jrc::UI::LOGIN);
}

// JS-callable setting changer: updates the AutoLoginCharacter config entry
// so the next auto-login selects the specified character. The setting is
// persisted to the Settings file and takes effect on the next UICharSelect.
EMSCRIPTEN_KEEPALIVE
int maple_set_auto_login_character(const char* name)
{
    if (!name)
        return 0;
    jrc::Setting<jrc::AutoLoginCharacter>::get().save(std::string(name));
    return 1;
}

// JS-callable local testing aid. This only changes the WASM client's handling
// of touch damage; the server implementation and character data are untouched.
EMSCRIPTEN_KEEPALIVE
void maple_set_godmode(int enabled)
{
    jrc::Stage::get().get_player().set_test_godmode(enabled != 0);
}

EMSCRIPTEN_KEEPALIVE
int maple_get_godmode()
{
    return jrc::Stage::get().get_player().is_test_godmode() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_teleport_to_npc(const char* name)
{
    if (!name)
    {
        return 0;
    }

    return jrc::Stage::get().teleport_player_to_npc(name) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_talk_to_nearest_npc()
{
    jrc::Stage& stage = jrc::Stage::get();
    return stage.get_npcs().talk_to_nearest(stage.get_player().get_position()) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_warp_to_map(int mapid)
{
    if (mapid < 0)
    {
        return 0;
    }

    jrc::ChangeMapPacket(false, mapid, "", false).dispatch();
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int maple_send_chat(const char* message)
{
    if (!message)
    {
        return 0;
    }

    jrc::GeneralChatPacket(message, false).dispatch();
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int maple_send_menu_action(int action)
{
    switch (action)
    {
    case jrc::KeyAction::CHARSTATS:
    case jrc::KeyAction::INVENTORY:
    case jrc::KeyAction::EQUIPS:
    case jrc::KeyAction::SKILLBOOK:
    case jrc::KeyAction::KEYCONFIG:
    case jrc::KeyAction::MINIMAP:
    case jrc::KeyAction::WORLDMAP:
    case jrc::KeyAction::PARTY:
    case jrc::KeyAction::MAINMENU:
    case jrc::KeyAction::QUESTLOG:
    case jrc::KeyAction::SYSTEMMENU:
        jrc::UI::get().send_menu(static_cast<jrc::KeyAction::Id>(action));
        return 1;
    default:
        return 0;
    }
}

EMSCRIPTEN_KEEPALIVE
int maple_send_quest_action(int action, int questid, int npcid)
{
    if (action < 0 || action > 5
        || questid < std::numeric_limits<int16_t>::min()
        || questid > std::numeric_limits<int16_t>::max())
    {
        return 0;
    }

    const jrc::Point<int16_t> position = jrc::Stage::get().get_player().get_position();
    jrc::QuestActionPacket(
        static_cast<int8_t>(action),
        static_cast<int16_t>(questid),
        npcid,
        position
    ).dispatch();
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_charselect_count()
{
    auto charselect = jrc::UI::get().get_element<jrc::UICharSelect>();
    return charselect ? charselect->get_character_count() : -1;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_charselect_selected_index()
{
    auto charselect = jrc::UI::get().get_element<jrc::UICharSelect>();
    return charselect ? charselect->get_selected_character_index() : -1;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_charselect_selected_id()
{
    auto charselect = jrc::UI::get().get_element<jrc::UICharSelect>();
    return charselect ? charselect->get_selected_character_id() : -1;
}

EMSCRIPTEN_KEEPALIVE
const char* maple_get_charselect_selected_name()
{
    auto charselect = jrc::UI::get().get_element<jrc::UICharSelect>();
    return charselect ? charselect->get_selected_character_name() : nullptr;
}

EMSCRIPTEN_KEEPALIVE
const char* maple_get_charselect_character_name(int index)
{
    auto charselect = jrc::UI::get().get_element<jrc::UICharSelect>();
    if (!charselect || index < 0 || index > std::numeric_limits<uint8_t>::max())
    {
        return nullptr;
    }
    return charselect->get_character_name(static_cast<uint8_t>(index));
}

EMSCRIPTEN_KEEPALIVE
int maple_select_character_slot(int index)
{
    auto charselect = jrc::UI::get().get_element<jrc::UICharSelect>();
    if (!charselect || index < 0 || index > std::numeric_limits<uint8_t>::max())
    {
        return 0;
    }
    return charselect->select_character(static_cast<uint8_t>(index)) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_select_character_by_name(const char* name)
{
    auto charselect = jrc::UI::get().get_element<jrc::UICharSelect>();
    if (!charselect || !name)
    {
        return 0;
    }
    return charselect->select_character_by_name(name) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_start_selected_character()
{
    auto charselect = jrc::UI::get().get_element<jrc::UICharSelect>();
    return charselect && charselect->start_selected_character() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
const char* maple_get_dialogue_text()
{
    const jrc::UINpcTalk* npctalk = active_npc_dialogue();
    return npctalk ? npctalk->get_dialogue_text_cstr() : nullptr;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_dialogue_npcid()
{
    const jrc::UINpcTalk* npctalk = active_npc_dialogue();
    return npctalk ? npctalk->get_dialogue_npcid() : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_dialogue_type()
{
    const jrc::UINpcTalk* npctalk = active_npc_dialogue();
    return npctalk ? npctalk->get_dialogue_type() : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_dialogue_mode()
{
    const jrc::UINpcTalk* npctalk = active_npc_dialogue();
    return npctalk ? npctalk->get_dialogue_mode() : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_dialogue_selection_count()
{
    const jrc::UINpcTalk* npctalk = active_npc_dialogue();
    return npctalk ? npctalk->get_dialogue_selection_count() : 0;
}

EMSCRIPTEN_KEEPALIVE
const char* maple_get_dialogue_selection(int index)
{
    const jrc::UINpcTalk* npctalk = active_npc_dialogue();
    return npctalk ? npctalk->get_dialogue_selection_cstr(index) : nullptr;
}

EMSCRIPTEN_KEEPALIVE
int maple_advance_dialogue(int action)
{
    jrc::UINpcTalk* npctalk = active_npc_dialogue_mutable();
    return npctalk && npctalk->simulate_button(action) ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_player_mapid()
{
    return jrc::Stage::get().get_mapid();
}

EMSCRIPTEN_KEEPALIVE
int maple_get_player_hp()
{
    return jrc::Stage::get().get_player().get_stats().get_stat(jrc::Maplestat::HP);
}

EMSCRIPTEN_KEEPALIVE
int maple_get_player_mp()
{
    return jrc::Stage::get().get_player().get_stats().get_stat(jrc::Maplestat::MP);
}

EMSCRIPTEN_KEEPALIVE
int maple_get_player_level()
{
    return jrc::Stage::get().get_player().get_stats().get_stat(jrc::Maplestat::LEVEL);
}

EMSCRIPTEN_KEEPALIVE
int maple_get_player_job()
{
    return jrc::Stage::get().get_player().get_stats().get_stat(jrc::Maplestat::JOB);
}

EMSCRIPTEN_KEEPALIVE
double maple_get_player_exp()
{
    return static_cast<double>(jrc::Stage::get().get_player().get_stats().get_exp());
}

EMSCRIPTEN_KEEPALIVE
const char* maple_get_player_name()
{
    return jrc::Stage::get().get_player().get_stats().get_name().c_str();
}

EMSCRIPTEN_KEEPALIVE
int maple_get_player_position_x()
{
    return jrc::Stage::get().get_player().get_position().x();
}

EMSCRIPTEN_KEEPALIVE
int maple_get_player_position_y()
{
    return jrc::Stage::get().get_player().get_position().y();
}

EMSCRIPTEN_KEEPALIVE
int maple_get_npc_count()
{
    return active_npc_count();
}

EMSCRIPTEN_KEEPALIVE
int maple_get_npc_info(int index, int* out_oid, int* out_x, int* out_y, char* out_name, int name_buf_size)
{
    const jrc::Npc* npc = active_npc_at(index);
    if (!npc)
    {
        return 0;
    }

    const jrc::Point<int16_t> position = npc->get_position();
    if (out_oid)
    {
        *out_oid = npc->get_oid();
    }
    if (out_x)
    {
        *out_x = position.x();
    }
    if (out_y)
    {
        *out_y = position.y();
    }
    copy_string_to_buffer(npc->get_name(), out_name, name_buf_size);
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_quest_status(int questid)
{
    if (questid < std::numeric_limits<int16_t>::min() || questid > std::numeric_limits<int16_t>::max())
    {
        return 3;
    }

    jrc::Questlog& questlog = jrc::Stage::get().get_player().get_quests();
    const int16_t id = static_cast<int16_t>(questid);
    if (questlog.is_completed(id))
    {
        return 2;
    }
    if (questlog.is_started(id))
    {
        return 1;
    }
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_ui_element_active(int type)
{
    if (type < 0 || type >= jrc::UIElement::NUM_TYPES)
    {
        return -1;
    }

    jrc::UIElement* element = jrc::UI::get().get_element(
        static_cast<jrc::UIElement::Type>(type)
    );
    if (!element)
    {
        return 0;
    }

    return element->is_active() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int maple_get_ui_button_bounds(
    int type,
    int button_id,
    int* out_left,
    int* out_top,
    int* out_right,
    int* out_bottom
)
{
    if (!out_left || !out_top || !out_right || !out_bottom
        || type < 0 || type >= jrc::UIElement::NUM_TYPES
        || button_id < 0 || button_id > std::numeric_limits<uint16_t>::max())
    {
        return 0;
    }

    jrc::UIElement* element = jrc::UI::get().get_element(
        static_cast<jrc::UIElement::Type>(type)
    );
    if (!element)
    {
        return 0;
    }

    jrc::Rectangle<int16_t> bounds;
    if (!element->get_button_bounds(static_cast<uint16_t>(button_id), bounds))
    {
        return 0;
    }

    *out_left = bounds.l();
    *out_top = bounds.t();
    *out_right = bounds.r();
    *out_bottom = bounds.b();
    return 1;
}
}
#endif

int main()
{
    jrc::start();
    return 0;
}
