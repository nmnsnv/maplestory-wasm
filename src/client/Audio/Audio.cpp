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
#include "Audio.h"

#include "../Configuration.h"

#ifndef MS_PLATFORM_WASM
#define WIN32_LEAN_AND_MEAN
#include "bass/bass.h"
#else
using HSTREAM = uint64_t;
using HCHANNEL = uint64_t;
using HSAMPLE = uint64_t;
using DWORD = uint32_t;
#endif

#include "nlnx/nx.hpp"
#include "nlnx/audio.hpp"

namespace jrc
{
    constexpr const char* Error::messages[];

    Sound::Sound(Name name) : id(soundids[name]) {}

    Sound::Sound(nl::node src) : id(add_sound(src)) {}

    Sound::Sound() : id(0) {}

    void Sound::play() const
    {
        if (id > 0)
        {
            play(id);
        }
    }

#include <cstdint>

    Error Sound::init() {
        if (!set_sfxvolume(100)) return Error::AUDIO; // Provide default to avoid warning
#ifndef MS_PLATFORM_WASM
        if (!BASS_Init(1, 44100, 0, nullptr, nullptr))
        {
            return Error::AUDIO;
        }
#endif

        nl::node uisrc = nl::nx::sound["UI.img"];

        add_sound(Sound::BUTTONCLICK, uisrc["BtMouseClick"]);
        add_sound(Sound::BUTTONOVER,  uisrc["BtMouseOver"]);
        add_sound(Sound::SELECTCHAR,  uisrc["CharSelect"]);

        nl::node gamesrc = nl::nx::sound["Game.img"];

        add_sound(Sound::GAMESTART, gamesrc["GameIn"]);
        add_sound(Sound::JUMP,      gamesrc["Jump"]);
        add_sound(Sound::DROP,      gamesrc["DropItem"]);
        add_sound(Sound::PICKUP,    gamesrc["PickUpItem"]);
        add_sound(Sound::PORTAL,    gamesrc["Portal"]);
        add_sound(Sound::LEVELUP,   gamesrc["LevelUp"]);

        uint8_t volume = Setting<SFXVolume>::get().load();

        if (!set_sfxvolume(volume))
        {
            return Error::AUDIO;
        }

        return Error::NONE;
    }

    void Sound::close()
    {
#ifndef MS_PLATFORM_WASM
        BASS_Free();
#endif
    }

    bool Sound::set_sfxvolume(uint8_t vol)
    {
#ifndef MS_PLATFORM_WASM
        return BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, vol * 100u) == TRUE;
#else
        return true;
#endif
    }

    void Sound::play(size_t id)
    {
        if (!samples.count(id))
        {
            return;
        }

#ifndef MS_PLATFORM_WASM
        HCHANNEL channel = BASS_SampleGetChannel(
            static_cast<HSAMPLE>(samples.at(id)),
            false
        );
        BASS_ChannelPlay(channel, true);
#endif
    }

    size_t Sound::add_sound(nl::node src)
    {
        nl::audio ad = src;

        const void* data = ad.data();

        if (data)
        {
            size_t id = ad.id();

#ifndef MS_PLATFORM_WASM
            samples[id] = BASS_SampleLoad(
                true,
                data,
                82,
                static_cast<DWORD>(ad.length()),
                4,
                BASS_SAMPLE_OVER_POS
            );
#else
            samples[id] = 0;
#endif

            return id;
        }
        else
        {
            return 0;
        }
    }

    void Sound::add_sound(Name name, nl::node src)
    {
        size_t id = add_sound(src);

        if (id)
        {
            soundids[name] = id;
        }
    }

    std::unordered_map<size_t, uint64_t> Sound::samples;
    EnumMap<Sound::Name, size_t> Sound::soundids;


    Music::Music(const std::string& p) : path(p) {}

    void Music::play() const
    {
        static HSTREAM stream = 0;
        static std::string bgmpath;

        if (path == bgmpath)
        {
            return;
        }

        nl::audio ad = nl::nx::sound.resolve(path);
        const void* data = ad.data();

        if (data)
        {
#ifndef MS_PLATFORM_WASM
            if (stream)
            {
                BASS_ChannelStop(stream);
                BASS_StreamFree(stream);
            }

            stream = BASS_StreamCreateFile(
                true,
                data,
                82,
                ad.length(),
                BASS_SAMPLE_FLOAT | BASS_SAMPLE_LOOP
            );
            BASS_ChannelPlay(stream, true);
#endif

            bgmpath = path;
        }
    }


    Error Music::init()
    {
        uint8_t volume = Setting<BGMVolume>::get().load();

        if (!set_bgmvolume(volume))
        {
            return Error::AUDIO;
        }

        return Error::NONE;
    }

    bool Music::set_bgmvolume(uint8_t vol)
    {
#ifndef MS_PLATFORM_WASM
        return BASS_SetConfig(BASS_CONFIG_GVOL_SAMPLE, vol * 100u) == TRUE;
#else
        return true;
#endif
    }
}
