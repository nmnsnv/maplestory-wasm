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
#include <emscripten.h>
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
    constexpr uint32_t kNxAudioHeaderSize = 82;

#ifdef MS_PLATFORM_WASM
    EM_JS(int, wasm_audio_init, (), {
        if (Module.MapleAudio) {
            return 1;
        }

        const AudioContextCtor = globalThis.AudioContext || globalThis.webkitAudioContext;
        if (!AudioContextCtor) {
            console.error("[audio] Web Audio is not available in this browser");
            return 0;
        }

        const context = new AudioContextCtor();
        const bgmGain = context.createGain();
        const sfxGain = context.createGain();
        bgmGain.connect(context.destination);
        sfxGain.connect(context.destination);

        const state = {
            context,
            bgmGain,
            sfxGain,
            clips: new Map(),
            currentMusic: null,
            musicToken: 0,
            unlockBound: false
        };

        const unlock = () => {
            if (state.context.state !== "running") {
                state.context.resume().catch((error) => {
                    console.debug("[audio] AudioContext resume deferred:", error);
                });
            }
        };

        const ensureUnlockHandlers = () => {
            if (state.unlockBound) {
                return;
            }

            state.unlockBound = true;
            const options = { passive: true };
            document.addEventListener("pointerdown", unlock, options);
            document.addEventListener("keydown", unlock, options);
            document.addEventListener("touchend", unlock, options);
        };

        const copyBytes = (ptr, len) => HEAPU8.slice(ptr, ptr + len);

        const decodeClip = (id, ptr, len) => {
            let clip = state.clips.get(id);
            if (!clip) {
                clip = {};
                state.clips.set(id, clip);
            }

            if (clip.buffer || clip.promise || clip.failed) {
                return clip.promise;
            }

            const bytes = copyBytes(ptr, len);
            clip.promise = state.context.decodeAudioData(bytes.buffer.slice(0)).then((buffer) => {
                clip.buffer = buffer;
                return buffer;
            }).catch((error) => {
                clip.failed = true;
                console.error("[audio] Failed to decode clip", id, error);
                throw error;
            });

            return clip.promise;
        };

        const playBuffer = (buffer, gainNode, loop) => {
            unlock();

            const source = state.context.createBufferSource();
            source.buffer = buffer;
            source.loop = loop;
            source.connect(gainNode);
            source.start(0);

            return source;
        };

        Module.MapleAudio = {
            unlock,

            close() {
                if (state.currentMusic) {
                    try {
                        state.currentMusic.stop();
                    } catch (error) {
                        console.debug("[audio] Ignoring stop failure during close:", error);
                    }
                    state.currentMusic.disconnect();
                    state.currentMusic = null;
                }
            },

            setSfxVolume(value) {
                state.sfxGain.gain.value = Math.max(0, Math.min(1, value));
            },

            setBgmVolume(value) {
                state.bgmGain.gain.value = Math.max(0, Math.min(1, value));
            },

            registerClip(id, ptr, len) {
                ensureUnlockHandlers();
                decodeClip(id, ptr, len);
            },

            playSound(id) {
                const clip = state.clips.get(id);
                if (!clip) {
                    return;
                }

                if (clip.buffer) {
                    playBuffer(clip.buffer, state.sfxGain, false);
                    return;
                }

                if (clip.promise && !clip.pendingPlay) {
                    clip.pendingPlay = true;
                    clip.promise.then(() => {
                        clip.pendingPlay = false;
                        if (clip.buffer) {
                            playBuffer(clip.buffer, state.sfxGain, false);
                        }
                    }).catch(() => {
                        clip.pendingPlay = false;
                    });
                }
            },

            playMusic(id) {
                const clip = state.clips.get(id);
                if (!clip) {
                    return;
                }

                state.musicToken += 1;
                const token = state.musicToken;

                if (state.currentMusic) {
                    try {
                        state.currentMusic.stop();
                    } catch (error) {
                        console.debug("[audio] Ignoring stop failure during music switch:", error);
                    }
                    state.currentMusic.disconnect();
                    state.currentMusic = null;
                }

                const startMusic = (buffer) => {
                    if (token != state.musicToken) {
                        return;
                    }

                    const source = playBuffer(buffer, state.bgmGain, true);
                    source.onended = () => {
                        if (state.currentMusic === source) {
                            state.currentMusic = null;
                        }
                    };
                    state.currentMusic = source;
                };

                if (clip.buffer) {
                    startMusic(clip.buffer);
                    return;
                }

                if (clip.promise) {
                    clip.promise.then((buffer) => {
                        startMusic(buffer);
                    }).catch(() => {});
                }
            }
        };

        return 1;
    });

    EM_JS(void, wasm_audio_close, (), {
        if (Module.MapleAudio) {
            Module.MapleAudio.close();
        }
    });

    EM_JS(int, wasm_audio_set_sfx_volume, (double volume), {
        if (!Module.MapleAudio) {
            return 0;
        }

        Module.MapleAudio.setSfxVolume(volume);
        return 1;
    });

    EM_JS(int, wasm_audio_set_bgm_volume, (double volume), {
        if (!Module.MapleAudio) {
            return 0;
        }

        Module.MapleAudio.setBgmVolume(volume);
        return 1;
    });

    EM_JS(int, wasm_audio_register_clip, (int id, const void* data, int length), {
        if (!Module.MapleAudio) {
            return 0;
        }

        Module.MapleAudio.registerClip(id, data, length);
        return 1;
    });

    EM_JS(void, wasm_audio_play_sound, (int id), {
        if (Module.MapleAudio) {
            Module.MapleAudio.playSound(id);
        }
    });

    EM_JS(void, wasm_audio_play_music, (int id), {
        if (Module.MapleAudio) {
            Module.MapleAudio.playMusic(id);
        }
    });
#else
    using HSTREAM = uint64_t;
    using HCHANNEL = uint64_t;
    using HSAMPLE = uint64_t;
    using DWORD = uint32_t;
#endif

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

    Error Sound::init() {
#ifndef MS_PLATFORM_WASM
        if (!set_sfxvolume(100)) return Error::AUDIO;
        if (!BASS_Init(1, 44100, 0, nullptr, nullptr))
        {
            return Error::AUDIO;
        }
#else
        if (!wasm_audio_init())
        {
            return Error::AUDIO;
        }

        if (!set_sfxvolume(100)) return Error::AUDIO;
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
#else
        wasm_audio_close();
#endif
    }

    bool Sound::set_sfxvolume(uint8_t vol)
    {
#ifndef MS_PLATFORM_WASM
        return BASS_SetConfig(BASS_CONFIG_GVOL_STREAM, vol * 100u) == TRUE;
#else
        return wasm_audio_set_sfx_volume(static_cast<double>(vol) / 100.0) != 0;
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
#else
        wasm_audio_play_sound(static_cast<int>(id));
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
                kNxAudioHeaderSize,
                static_cast<DWORD>(ad.length()),
                4,
                BASS_SAMPLE_OVER_POS
            );
#else
            if (ad.length() <= kNxAudioHeaderSize)
            {
                return 0;
            }

            samples[id] = id;
            wasm_audio_register_clip(
                static_cast<int>(id),
                static_cast<uint8_t const*>(data) + kNxAudioHeaderSize,
                static_cast<int>(ad.length() - kNxAudioHeaderSize)
            );
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
                kNxAudioHeaderSize,
                ad.length(),
                BASS_SAMPLE_FLOAT | BASS_SAMPLE_LOOP
            );
            BASS_ChannelPlay(stream, true);
#else
            if (ad.length() <= kNxAudioHeaderSize)
            {
                return;
            }

            size_t id = ad.id();
            wasm_audio_register_clip(
                static_cast<int>(id),
                static_cast<uint8_t const*>(data) + kNxAudioHeaderSize,
                static_cast<int>(ad.length() - kNxAudioHeaderSize)
            );
            wasm_audio_play_music(static_cast<int>(id));
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
        return wasm_audio_set_bgm_volume(static_cast<double>(vol) / 100.0) != 0;
#endif
    }
}
