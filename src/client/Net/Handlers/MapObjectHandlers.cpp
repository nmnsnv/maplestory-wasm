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
#include "MapObjectHandlers.h"

#include "Helpers/LoginParser.h"
#include "Helpers/MobTemporaryStatMasks.h"
#include "Helpers/MovementParser.h"

#include "../../Audio/Audio.h"
#include "../../Gameplay/Stage.h"
#include "../../Gameplay/Spawn.h"

namespace jrc
{
    namespace
    {
        bool has_mob_temporary_status(
            const std::array<int32_t, MobTemporaryStatMasks::MASK_COUNT>& masks,
            const MobTemporaryStatMasks::Descriptor& status)
        {
            size_t index = status.first_mask
                ? MobTemporaryStatMasks::FIRST_MASK_INDEX
                : MobTemporaryStatMasks::SECOND_MASK_INDEX;
            return (masks[index] & status.value) != 0;
        }

        void skip_mob_temporary_data(InPacket& recv)
        {
            // Cosmic writes a 16-byte mask here and then appends per-status payloads.
            // Skipping a fixed size shifts the mob position whenever the status block changes.
            std::array<int32_t, MobTemporaryStatMasks::MASK_COUNT> masks{{
                recv.read_int(),
                recv.read_int(),
                recv.read_int(),
                recv.read_int()
            }};

            bool has_weapon_reflect = false;
            bool has_magic_reflect = false;

            for (const auto& status : MobTemporaryStatMasks::ALL)
            {
                if (!has_mob_temporary_status(masks, status))
                {
                    continue;
                }

                recv.skip(MobTemporaryStatMasks::STATUS_PAYLOAD_SIZE);

                if (status.value == MobTemporaryStatMasks::WEAPON_REFLECT)
                {
                    has_weapon_reflect = true;
                }
                else if (status.value == MobTemporaryStatMasks::MAGIC_REFLECT)
                {
                    has_magic_reflect = true;
                }
            }

            if (has_weapon_reflect)
            {
                recv.skip(MobTemporaryStatMasks::REFLECT_COUNTER_SIZE);
            }

            if (has_magic_reflect)
            {
                recv.skip(MobTemporaryStatMasks::REFLECT_COUNTER_SIZE);
            }

            if (has_weapon_reflect || has_magic_reflect)
            {
                recv.skip(MobTemporaryStatMasks::REFLECT_COUNTER_SIZE);
            }
        }

        void spawn_mob_from_packet(InPacket& recv, int32_t oid, int32_t id, int8_t mode)
        {
            skip_mob_temporary_data(recv);

            Point<int16_t> position = recv.read_point();
            int8_t stance = recv.read_byte();

            if (recv.length() >= 2)
            {
                recv.skip(2); // origin foothold
            }

            uint16_t fh = recv.length() >= 2 ? recv.read_short() : 0;
            int8_t effect = 0;
            int8_t team = -1;

            // Fake mobs use a short here instead of the regular spawn-effect byte.
            if (recv.length() == 7 && recv.inspect_short() == -2)
            {
                effect = static_cast<int8_t>(recv.read_short());
                team = recv.length() >= 1 ? recv.read_byte() : -1;
            }
            else if (recv.length() >= 1)
            {
                effect = recv.read_byte();

                if (effect == -3 && recv.length() >= 4)
                {
                    recv.read_int(); // parent mob oid
                }
                else if (effect > 0 && recv.length() >= 3)
                {
                    recv.skip(3);
                    if (effect == 15 && recv.length() >= 1)
                    {
                        recv.skip(1);
                    }
                }

                team = recv.length() >= 1 ? recv.read_byte() : -1;
            }

            if (recv.length() >= 4)
            {
                recv.skip(4); // item effect
            }

            Stage::get().get_mobs().spawn({
                oid, id, mode, stance, fh, effect == -2, team, position
            });
        }
    }

    void SpawnCharHandler::handle(InPacket& recv) const
    {
        int32_t cid = recv.read_int();
        uint8_t level = recv.read_byte();
        std::string name = recv.read_string();

        recv.read_string(); // guildname
        recv.read_short(); // guildlogobg
        recv.read_byte(); // guildlogobgcolor
        recv.read_short(); // guildlogo
        recv.read_byte(); // guildlogocolor

        recv.skip(8);

        bool morphed = recv.read_int() == 2;
        int32_t buffmask1 = recv.read_int();
        int16_t buffvalue = 0;
        if (buffmask1 != 0)
        {
            buffvalue = morphed ? recv.read_short() : recv.read_byte();
        }
        recv.read_int(); // buffmask 2

        recv.skip(43);

        recv.read_int(); // 'mount'

        recv.skip(61);

        int16_t job = recv.read_short();
        LookEntry look = LoginParser::parse_look(recv);

        recv.read_int(); //count of 5110000
        recv.read_int(); // 'itemeffect'
        recv.read_int(); // 'chair'

        Point<int16_t> position = recv.read_point();
        int8_t stance = recv.read_byte();

        recv.skip(3);

        for (size_t i = 0; i < 3; i++)
        {
            int8_t available = recv.read_byte();
            if (available == 1)
            {
                recv.read_byte(); // 'byte2'
                recv.read_int(); // petid
                recv.read_string(); // name
                recv.read_int(); // unique id
                recv.read_int();
                recv.read_point(); // pos
                recv.read_byte(); // stance
                recv.read_int(); // fhid
            }
            else
            {
                break;
            }
        }

        recv.read_int(); // mountlevel
        recv.read_int(); // mountexp
        recv.read_int(); // mounttiredness

        //shop stuff, TO DO
        recv.read_byte();
        //shop stuff end

        bool chalkboard = recv.read_bool();
        std::string chalktext = chalkboard ? recv.read_string() : "";

        recv.skip(3);
        recv.read_byte(); // team

        Stage::get().get_chars().spawn({
            cid, look, level, job, name, stance, position
        });
    }


    void RemoveCharHandler::handle(InPacket& recv) const
    {
        int32_t cid = recv.read_int();

        Stage::get().get_chars().remove(cid);
    }


    void SpawnPetHandler::handle(InPacket& recv) const
    {
        int32_t cid = recv.read_int();
        Optional<Char> character = Stage::get()
            .get_character(cid);
        if (!character)
            return;

        uint8_t petindex = recv.read_byte();
        int8_t mode = recv.read_byte();

        if (mode == 1)
        {
            recv.skip(1);

            int32_t itemid = recv.read_int();
            std::string name = recv.read_string();
            int32_t uniqueid = recv.read_int();

            recv.skip(4);

            Point<int16_t> pos = recv.read_point();
            uint8_t stance = recv.read_byte();
            int32_t fhid = recv.read_int();

            character->add_pet(petindex, itemid, name, uniqueid, pos, stance, fhid);
        }
        else if (mode == 0)
        {
            bool hunger = recv.read_bool();

            character->remove_pet(petindex, hunger);
        }
    }


    void CharMovedHandler::handle(InPacket& recv) const
    {
        int32_t cid = recv.read_int();
        recv.skip(4);
        std::vector<Movement> movements = MovementParser::parse_movements(recv);

        Stage::get().get_chars().send_movement(cid, movements);
    }


    void UpdateCharLookHandler::handle(InPacket& recv) const
    {
        int32_t cid = recv.read_int();
        recv.read_byte();
        LookEntry look = LoginParser::parse_look(recv);

        Stage::get().get_chars().update_look(cid, look);
    }


    void ShowForeignEffectHandler::handle(InPacket& recv) const
    {
        int32_t cid = recv.read_int();
        int8_t effect = recv.read_byte();
        if (effect == 10) // recovery
        {
            recv.read_byte(); // 'amount'
        }
        else if (effect == 13) // card effect
        {
            Stage::get()
                .show_character_effect(cid, CharEffect::MONSTER_CARD);
        }
        else if (recv.available()) // skill
        {
            int32_t skillid = recv.read_int();
            recv.read_byte(); // 'direction'
            // 9 more bytes after this

            Stage::get().get_combat().show_buff(cid, skillid, effect);
        }
        else
        {
            // todo
        }
    }


    void SpawnMobHandler::handle(InPacket& recv) const
    {
        int32_t oid = recv.read_int();
        recv.read_byte(); // 5 if controller == null
        int32_t id = recv.read_int();
        spawn_mob_from_packet(recv, oid, id, 0);
    }


    void KillMobHandler::handle(InPacket& recv) const
    {
        int32_t oid = recv.read_int();
        int8_t animation = recv.read_byte();

        Stage::get().get_mobs().remove(oid, animation);
    }


    void SpawnMobControllerHandler::handle(InPacket& recv) const
    {
        int8_t mode = recv.read_byte();
        int32_t oid = recv.read_int();
        if (mode == 0)
        {
            Stage::get().get_mobs().set_control(oid, false);
        }
        else
        {
            if (recv.available())
            {
                recv.skip(1);

                int32_t id = recv.read_int();
                spawn_mob_from_packet(recv, oid, id, mode);
            }
            else
            {
                // remove monster invisibility, not used in moopledev or solaxia (maybe in an event script?)
            }
        }
    }


    void MobMovedHandler::handle(InPacket& recv) const
    {
        int32_t oid = recv.read_int();

        recv.read_byte();
        recv.read_byte(); // useskill
        recv.read_byte(); // skill
        recv.read_byte(); // skill 1
        recv.read_byte(); // skill 2
        recv.read_byte(); // skill 3
        recv.read_byte(); // skill 4

        Point<int16_t> position = recv.read_point();
        std::vector<Movement> movements = MovementParser::parse_movements(recv);

        Stage::get().get_mobs().send_movement(oid, position, std::move(movements));
    }


    void ShowMobHpHandler::handle(InPacket& recv) const
    {
        int32_t oid = recv.read_int();
        int8_t hppercent = recv.read_byte();
        uint16_t playerlevel = Stage::get().get_player().get_stats().get_stat(Maplestat::LEVEL);

        Stage::get().get_mobs().send_mobhp(oid, hppercent, playerlevel);
    }


    void SpawnNpcHandler::handle(InPacket& recv) const
    {
        int32_t oid = recv.read_int();
        int32_t id = recv.read_int();
        Point<int16_t> position = recv.read_point();
        bool flip = recv.read_bool();
        uint16_t fh = recv.read_short();

        recv.read_short(); // 'rx'
        recv.read_short(); // 'ry'

        Stage::get().get_npcs().spawn({
            oid, id, position, flip, fh
        });
    }


    void SpawnNpcControllerHandler::handle(InPacket& recv) const
    {
        int8_t mode = recv.read_byte();
        int32_t oid = recv.read_int();

        if (mode == 0)
        {
            Stage::get().get_npcs().remove(oid);
        }
        else
        {
            int32_t id = recv.read_int();
            Point<int16_t> position = recv.read_point();
            bool flip = recv.read_bool();
            uint16_t fh = recv.read_short();

            recv.read_short(); // 'rx'
            recv.read_short(); // 'ry'
            recv.read_bool(); // 'minimap'

            Stage::get().get_npcs().spawn({
                oid, id, position, flip, fh
            });
        }
    }


    void DropLootHandler::handle(InPacket& recv) const
    {
        int8_t mode = recv.read_byte();
        int32_t oid = recv.read_int();
        bool meso = recv.read_bool();
        int32_t itemid = recv.read_int();
        int32_t owner = recv.read_int();
        int8_t pickuptype = recv.read_byte();
        Point<int16_t> dropto = recv.read_point();

        recv.skip(4);

        Point<int16_t> dropfrom;
        if (mode != 2)
        {
            dropfrom = recv.read_point();

            recv.skip(2);
        }
        else
        {
            dropfrom = dropto;
        }

        if (!meso)
        {
            recv.skip(8);
        }
        bool playerdrop = !recv.read_bool();

        Stage::get().get_drops().spawn({
            oid, itemid, meso, owner, dropfrom, dropto, pickuptype, mode, playerdrop
        });
    }


    void RemoveLootHandler::handle(InPacket& recv) const
    {
        int8_t mode = recv.read_byte();
        int32_t oid = recv.read_int();

        Optional<PhysicsObject> looter;
        if (mode > 1)
        {
            int32_t cid = recv.read_int();
            if (recv.length() > 0)
            {
                recv.read_byte(); // pet
            }
            else if (auto character = Stage::get().get_character(cid))
            {
                looter = character->get_phobj();
            }

            Sound(Sound::PICKUP).play();
        }

        Stage::get().get_drops().remove(oid, mode, looter.get());
    }


    void SpawnReactorHandler::handle(InPacket& recv) const
    {
        int32_t oid = recv.read_int();
        int32_t rid = recv.read_int();
        int8_t state = recv.read_byte();
        Point<int16_t> point = recv.read_point();

        // Unused in Solaxia
        // uint16_t fhid = recv.read_short();
        // recv.read_byte()

        Stage::get().get_reactors().spawn({
            oid, rid, state, point
        });
    }


    void RemoveReactorHandler::handle(InPacket& recv) const
    {
        int32_t oid = recv.read_int();
        int8_t state = recv.read_byte();
        Point<int16_t> point = recv.read_point();

        Stage::get().get_reactors()
            .remove(oid, state, point);
    }
}
