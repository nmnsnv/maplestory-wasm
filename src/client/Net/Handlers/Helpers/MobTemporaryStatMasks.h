#pragma once

#include <array>
#include <cstddef>
#include <cstdint>


namespace jrc
{
    namespace MobTemporaryStatMasks
    {
        struct Descriptor
        {
            int32_t value;
            bool first_mask;
        };

        constexpr std::size_t MASK_COUNT = 4;
        constexpr std::size_t FIRST_MASK_INDEX = 0;
        constexpr std::size_t SECOND_MASK_INDEX = 2;
        constexpr std::size_t STATUS_PAYLOAD_SIZE = 8;
        constexpr std::size_t REFLECT_COUNTER_SIZE = 4;

        constexpr int32_t NEUTRALISE        = 0x2;
        constexpr int32_t PHANTOM_IMPRINT   = 0x4;
        constexpr int32_t MATK              = 0x4;
        constexpr int32_t MDEF              = 0x8;
        constexpr int32_t ACC               = 0x10;
        constexpr int32_t AVOID             = 0x20;
        constexpr int32_t SPEED             = 0x40;
        constexpr int32_t STUN              = 0x80;
        constexpr int32_t FREEZE            = 0x100;
        constexpr int32_t POISON            = 0x200;
        constexpr int32_t SEAL              = 0x400;
        constexpr int32_t SHOWDOWN          = 0x800;
        constexpr int32_t WEAPON_ATTACK_UP  = 0x1000;
        constexpr int32_t WEAPON_DEFENSE_UP = 0x2000;
        constexpr int32_t MAGIC_ATTACK_UP   = 0x4000;
        constexpr int32_t MAGIC_DEFENSE_UP  = 0x8000;
        constexpr int32_t DOOM              = 0x10000;
        constexpr int32_t SHADOW_WEB        = 0x20000;
        constexpr int32_t WEAPON_IMMUNITY   = 0x40000;
        constexpr int32_t MAGIC_IMMUNITY    = 0x80000;
        constexpr int32_t HARD_SKIN         = 0x200000;
        constexpr int32_t NINJA_AMBUSH      = 0x400000;
        constexpr int32_t ELEMENTAL_ATTR    = 0x800000;
        constexpr int32_t VENOMOUS_WEAPON   = 0x1000000;
        constexpr int32_t BLIND             = 0x2000000;
        constexpr int32_t SEAL_SKILL        = 0x4000000;
        constexpr int32_t INERT_MOB         = 0x10000000;
        constexpr int32_t WEAPON_REFLECT    = 0x20000000;
        constexpr int32_t MAGIC_REFLECT     = 0x40000000;

        constexpr std::array<Descriptor, 29> ALL{{
            { NEUTRALISE, true },
            { PHANTOM_IMPRINT, true },
            { MATK, false },
            { MDEF, false },
            { ACC, false },
            { AVOID, false },
            { SPEED, false },
            { STUN, false },
            { FREEZE, false },
            { POISON, false },
            { SEAL, false },
            { SHOWDOWN, false },
            { WEAPON_ATTACK_UP, false },
            { WEAPON_DEFENSE_UP, false },
            { MAGIC_ATTACK_UP, false },
            { MAGIC_DEFENSE_UP, false },
            { DOOM, false },
            { SHADOW_WEB, false },
            { WEAPON_IMMUNITY, false },
            { MAGIC_IMMUNITY, false },
            { HARD_SKIN, false },
            { NINJA_AMBUSH, false },
            { ELEMENTAL_ATTR, false },
            { VENOMOUS_WEAPON, false },
            { BLIND, false },
            { SEAL_SKILL, false },
            { INERT_MOB, false },
            { WEAPON_REFLECT, true },
            { MAGIC_REFLECT, true }
        }};
    }
}
