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
#include "UISkillBook.h"

#include "../Components/MapleButton.h"
#include "../Components/TwoSpriteButton.h"
#include "UIKeyConfig.h"

#include "../../Character/SkillId.h"
#include "../../Data/JobData.h"
#include "../../Data/SkillData.h"
#include "../../Gameplay/Stage.h"
#include "../../Net/Packets/PlayerPackets.h"
#include "../../IO/UI.h"
#include "../../Util/Misc.h"

#include "nlnx/nx.hpp"


namespace jrc
{
    namespace
    {
        class SkillDragType : public Icon::Type
        {
        public:
            explicit SkillDragType(int32_t in_skill_id)
                : skill_id(in_skill_id) {}

            void drop_on_stage() const override {}
            void drop_on_equips(Equipslot::Id) const override {}
            void drop_on_items(InventoryType::Id, Equipslot::Id, int16_t, bool) const override {}
            void drop_on_bindings(Point<int16_t> cursorposition, bool remove) const override
            {
                if (auto keyconfig = UI::get().get_element<UIKeyConfig>())
                {
                    Keyboard::Mapping mapping{ KeyType::SKILL, skill_id };
                    if (remove)
                        keyconfig->unstage_mapping(mapping);
                    else
                        keyconfig->stage_mapping(cursorposition, mapping);
                }
            }

        private:
            int32_t skill_id;
        };
    }

    constexpr Point<int16_t> UISkillbook::SKILL_OFFSET;
    constexpr Point<int16_t> UISkillbook::ICON_OFFSET;
    constexpr Point<int16_t> UISkillbook::LINE_OFFSET;
    constexpr Point<int16_t> SKILL_ICON_HIT_ADJUST = { 0, -32 };

    SkillIcon::SkillIcon(int32_t i, int32_t lv) : id(i)
    {
        const SkillData& data = SkillData::get(id);

        normal    = data.get_icon(SkillData::NORMAL);
        mouseover = data.get_icon(SkillData::MOUSEOVER);
        disabled  = data.get_icon(SkillData::DISABLED);
        drag_icon = std::make_unique<Icon>(
            std::make_unique<SkillDragType>(id),
            normal,
            -1
        );

        std::string namestr  = data.get_name();
        std::string levelstr = std::to_string(lv);

        name  = { Text::A11L, Text::LEFT, Text::DARKGREY, namestr  };
        level = { Text::A11L, Text::LEFT, Text::DARKGREY, levelstr };
        state = NORMAL;

        constexpr uint16_t MAX_NAME_WIDTH = 96;
        size_t overhang = 3;
        while (name.width() > MAX_NAME_WIDTH)
        {
            namestr.replace(namestr.end() - overhang, namestr.end(), "...");
            overhang++;

            name.change_text(namestr);
        }
    }

    void SkillIcon::draw(const DrawArgument& args) const
    {
        switch (state)
        {
            case NORMAL:
                normal.draw(args);
                break;
            case DISABLED:
                disabled.draw(args);
                break;
            case MOUSEOVER:
                mouseover.draw(args);
                break;
        }

        name.draw(args  + Point<int16_t>(38, -34));
        level.draw(args + Point<int16_t>(38, -16));
    }

    Cursor::State SkillIcon::send_cursor(Point<int16_t> cursorpos, bool clicked)
    {
        constexpr Rectangle<int16_t> bounds(0, 32, 0, 32);
        bool inrange = bounds.contains(cursorpos);
        switch (state)
        {
        case NORMAL:
        case DISABLED:
            if (inrange)
            {
                if (clicked)
                {
                    state = MOUSEOVER;
                    return Cursor::GRABBING;
                }
                else
                {
                    state = MOUSEOVER;
                    return Cursor::CANGRAB;
                }
            }
            else
            {
                return Cursor::IDLE;
            }
        case MOUSEOVER:
            if (inrange)
            {
                if (clicked)
                {
                    state = MOUSEOVER;
                    return Cursor::GRABBING;
                }
                else
                {
                    state = MOUSEOVER;
                    return Cursor::CANGRAB;
                }
            }
            else
            {
                state = NORMAL;
                return Cursor::IDLE;
            }
        default:
            return Cursor::IDLE;
        }
    }

    int32_t SkillIcon::get_id() const
    {
        return id;
    }

    Icon* SkillIcon::get_drag_icon() const
    {
        return drag_icon.get();
    }

    UISkillbook::UISkillbook(const CharStats& in_stats,
                             const Skillbook& in_skillbook)
        : UIDragElement({ 174, 20 }),
          stats(in_stats),
          skillbook(in_skillbook),
          sp(0),
          beginner_sp(0),
          tab(0),
          skillcount(0),
          offset(0),
          grabbing(false)
    {
        nl::node main = nl::nx::ui["UIWindow2.img"]["Skill"]["main"];

        sprites.emplace_back(main["backgrnd"]);
        sprites.emplace_back(main["backgrnd2"]);
        sprites.emplace_back(main["backgrnd3"]);

        skilld = main["skill0"];
        skille = main["skill1"];
        line   = main["line"];

        nl::node tabe = main["Tab"]["enabled"];
        nl::node tabd = main["Tab"]["disabled"];

        for (uint16_t i = BT_TAB0; i <= BT_TAB4; ++i)
        {
            uint16_t tabid = i - BT_TAB0;
            buttons[i] = std::make_unique<TwoSpriteButton>(tabd[tabid], tabe[tabid]);
        }
        for (uint16_t i = BT_SPUP0; i <= BT_SPUP3; ++i)
        {
            uint16_t spupid = i - BT_SPUP0;
            Point<int16_t> spup_position = SKILL_OFFSET + Point<int16_t>(124, 20 + spupid * ROW_HEIGHT);
            buttons[i] = std::make_unique<MapleButton>(main["BtSpUp"], spup_position);
        }

        booktext = { Text::A12M, Text::CENTER, Text::WHITE,     "", 100 };
        splabel  = { Text::A11M, Text::RIGHT,  Text::LIGHTGREY          };

        slider = {
            11, { 92, 236 }, 154, ROWS, 1,
            [&](bool upwards) {
                int16_t shift = upwards ? -1 : 1;
                bool above = offset + shift >= 0;
                bool below = offset + 4 + shift <= skillcount;
                if (above && below)
                {
                    change_offset(offset + shift);
                }
            }
        };

        change_job(stats.get_stat(Maplestat::JOB));
        change_sp(stats.get_stat(Maplestat::SP));

        dimension = { 174, 299 };
    }

    void UISkillbook::draw(float alpha) const
    {
        draw_sprites(alpha);

        bookicon.draw(position + Point<int16_t>(12, 85));
        booktext.draw(position + Point<int16_t>(100, 49));
        splabel.draw(position + Point<int16_t>(162, 254));

        auto begin = icons.begin();
        if (icons.size() > offset)
        {
            begin = begin + offset;
        }

        auto end = icons.end();
        if (icons.size() > ROWS + offset)
        {
            end = begin + ROWS;
        }

        Point<int16_t> skill_position = position + SKILL_OFFSET;
        for (auto iter = begin; iter != end; ++iter)
        {
            skille.draw(skill_position);
            iter->draw(skill_position + ICON_OFFSET);
            if (iter != end - 1)
            {
                line.draw(skill_position + LINE_OFFSET);
            }

            skill_position.shift_y(ROW_HEIGHT);
        }

        draw_buttons(alpha);

        slider.draw(position);
    }

    Button::State UISkillbook::button_pressed(uint16_t id)
    {
        switch (id)
        {
        case BT_TAB0:
        case BT_TAB1:
        case BT_TAB2:
        case BT_TAB3:
        case BT_TAB4:
            change_tab(id - BT_TAB0);
            return Button::PRESSED;
        case BT_SPUP0:
        case BT_SPUP1:
        case BT_SPUP2:
        case BT_SPUP3:
            send_spup(id - BT_SPUP0 + offset);
            return Button::PRESSED;
        default:
            return Button::PRESSED;
        }
    }

    void UISkillbook::doubleclick(Point<int16_t> cursorpos)
    {
        const SkillIcon* icon = icon_by_position(cursorpos - position);
        if (icon)
        {
            int32_t skill_id    = icon->get_id();
            int32_t skill_level = skillbook.get_level(skill_id);
            if (skill_level > 0)
            {
                Stage::get().get_combat().use_move(skill_id);
            }
        }
    }

    bool UISkillbook::remove_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (UIDragElement::remove_cursor(clicked, cursorpos))
        {
            return true;
        }

        return slider.remove_cursor(clicked);
    }

    UIElement::CursorResult UISkillbook::send_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (dragged)
        {
            return UIDragElement::send_cursor(clicked, cursorpos);
        }

        Point<int16_t> cursor_relative = cursorpos - position;
        if (slider.isenabled())
        {
            if (Cursor::State new_state = slider.send_cursor(cursor_relative, clicked))
            {
                clear_tooltip();
                return { new_state, true };
            }
        }

        auto begin = icons.begin();
        if (icons.size() > offset)
        {
            begin = begin + offset;
        }

        auto end = icons.end();
        if (icons.size() > ROWS + offset)
        {
            end = begin + ROWS;
        }

        Point<int16_t> skill_position = position + SKILL_OFFSET;
        if (!grabbing)
        {
            for (auto iter = begin; iter != end; ++iter)
            {
                Point<int16_t> icon_position = skill_position + ICON_OFFSET;
                Point<int16_t> icon_hit_position = icon_position + SKILL_ICON_HIT_ADJUST;
                if (Cursor::State state = iter->send_cursor(cursorpos - icon_hit_position, clicked))
                {
                    switch (state)
                    {
                    case Cursor::GRABBING:
                    {
                        clear_tooltip();
                        grabbing = true;

                        int32_t skill_id = iter->get_id();
                        int32_t skill_level = skillbook.get_level(skill_id);
                        if (skill_level > 0 && !SkillData::get(skill_id).is_passive())
                        {
                            if (Icon* icon = iter->get_drag_icon())
                            {
                                icon->start_drag(cursorpos - icon_hit_position);
                                UI::get().drag_icon(icon);
                            }

                            return { Cursor::GRABBING, true };
                        }

                        return { Cursor::IDLE, true };
                    }
                    case Cursor::CANGRAB:
                        show_skill(iter->get_id());
                        return { Cursor::IDLE, true };
                    default:
                        break;
                    }
                }

                skill_position.shift_y(ROW_HEIGHT);
            }

            clear_tooltip();
        }
        else
        {
            grabbing = false;
        }

        return UIDragElement::send_cursor(clicked, cursorpos);
    }

    void UISkillbook::send_key(int32_t, bool pressed, bool escape)
    {
        if (pressed && escape)
        {
            clear_tooltip();
            deactivate();
        }
    }

    void UISkillbook::update_stat(Maplestat::Id stat, int16_t value)
    {
        switch (stat)
        {
        case Maplestat::JOB:
            change_job(value);
            break;
        case Maplestat::SP:
            change_sp(value);
            break;
        case Maplestat::LEVEL:
            update_sp_label();
            change_offset(offset);
            break;
        default:
            return;
        }
    }

    void UISkillbook::update_skills(int32_t skill_id)
    {
        if (skill_id / 10000 == job.get_id())
        {
            change_tab(tab);
        }
    }

    void UISkillbook::change_job(uint16_t id)
    {
        job.change_job(id);

        Job::Level level = job.get_level();
        for (uint16_t i = 0; i <= Job::FOURTH; ++i)
        {
            buttons[BT_TAB0 + i]->set_active(i <= level);
        }

        change_tab(level - Job::BEGINNER);
    }

    void UISkillbook::change_sp(int16_t s)
    {
        sp = s;
        update_sp_label();

        change_offset(offset);
    }

    int16_t UISkillbook::get_beginner_sp() const
    {
        int16_t level = stats.get_stat(Maplestat::LEVEL);
        int16_t remaining = level - 1;
        if (remaining < 0)
        {
            remaining = 0;
        }
        if (remaining > 6)
        {
            remaining = 6;
        }

        remaining -= skillbook.get_level(SkillId::THREE_SNAILS);
        remaining -= skillbook.get_level(SkillId::HEAL);
        remaining -= skillbook.get_level(SkillId::FEATHER);

        return remaining > 0 ? remaining : 0;
    }

    int16_t UISkillbook::get_available_sp() const
    {
        Job::Level joblevel = joblevel_by_tab(tab);
        return joblevel == Job::BEGINNER ? beginner_sp : sp;
    }

    void UISkillbook::update_sp_label()
    {
        beginner_sp = get_beginner_sp();
        splabel.change_text(std::to_string(get_available_sp()));
    }

    void UISkillbook::change_tab(uint16_t new_tab)
    {
        buttons[BT_TAB0 + tab    ]->set_state(Button::NORMAL);
        buttons[BT_TAB0 + new_tab]->set_state(Button::PRESSED);
        tab = new_tab;

        icons.clear();
        skillcount = 0;

        Job::Level joblevel = joblevel_by_tab(tab);
        uint16_t subid = job.get_subjob(joblevel);

        const JobData& data = JobData::get(subid);

        bookicon = data.get_icon();
        booktext.change_text(data.get_name());

        for (int32_t skill_id : data.get_skills())
        {
            int32_t level = skillbook.get_level(skill_id);
            int32_t masterlevel = skillbook.get_masterlevel(skill_id);

            bool invisible = SkillData::get(skill_id).is_invisible();
            if (invisible && masterlevel == 0)
            {
                continue;
            }

            icons.emplace_back(skill_id, level);
            skillcount++;
        }

        slider.setrows(ROWS, skillcount);
        update_sp_label();
        change_offset(0);
    }

    void UISkillbook::change_offset(uint16_t new_offset)
    {
        offset = new_offset;

        for (int16_t i = 0; i < ROWS; ++i)
        {
            uint16_t index = BT_SPUP0 + i;
            uint16_t row = offset + i;
            buttons[index]->set_active(row < skillcount);
            if (row < icons.size())
            {
                int32_t skill_id = icons[row].get_id();
                bool canraise = can_raise(skill_id);
                buttons[index]->set_state(canraise ? Button::NORMAL : Button::DISABLED);
            }
        }
    }

    void UISkillbook::show_skill(int32_t id)
    {
        int32_t skill_id = id;
        int32_t level = skillbook.get_level(id);
        int32_t masterlevel = skillbook.get_masterlevel(id);
        int64_t expiration = skillbook.get_expiration(id);

        UI::get().show_skill(
            Tooltip::SKILLBOOK,
            skill_id,
            level,
            masterlevel,
            expiration
        );
    }

    void UISkillbook::clear_tooltip()
    {
        UI::get().clear_tooltip(Tooltip::SKILLBOOK);
    }

    bool UISkillbook::can_raise(int32_t skill_id) const
    {
        if (get_available_sp() <= 0)
        {
            return false;
        }

        int32_t level = skillbook.get_level(skill_id);
        int32_t masterlevel = skillbook.get_masterlevel(skill_id);
        if (masterlevel == 0)
        {
            masterlevel = SkillData::get(skill_id).get_masterlevel();
        }

        if (level >= masterlevel)
        {
            return false;
        }

        switch (skill_id)
        {
        case SkillId::ANGEL_BLESSING:
            return false;
        default:
            return true;
        }
    }

    void UISkillbook::send_spup(uint16_t row)
    {
        if (row >= icons.size())
        {
            return;
        }

        int32_t skill_id = icons[row].get_id();

        SpendSpPacket(skill_id).dispatch();
        UI::get().disable();
    }

    Job::Level UISkillbook::joblevel_by_tab(uint16_t t) const
    {
        switch (t)
        {
        case 1:
            return Job::FIRST;
        case 2:
            return Job::SECOND;
        case 3:
            return Job::THIRD;
        case 4:
            return Job::FOURTH;
        default:
            return Job::BEGINNER;
        }
    }

    SkillIcon* UISkillbook::icon_by_position(Point<int16_t> cursorpos)
    {
        int16_t x = cursorpos.x();
        if (x < SKILL_OFFSET.x() || x > 148)
        {
            return nullptr;
        }

        int16_t y = cursorpos.y();
        if (y < SKILL_OFFSET.y())
        {
            return nullptr;
        }

        uint16_t row = (y - SKILL_OFFSET.y()) / ROW_HEIGHT;
        if (row < 0 || row >= ROWS)
        {
            return nullptr;
        }

        uint16_t absrow = offset + row;
        if (icons.size() <= absrow)
        {
            return nullptr;
        }

        auto iter = icons.begin() + absrow;
        return icons.data() + (iter - icons.begin());
    }
}
