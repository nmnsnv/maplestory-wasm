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
#include "Texture.h"
#include "GraphicsGL.h"
#include "DrawBounds.h"

#include "../Configuration.h"

#include "nlnx/nx.hpp"

namespace jrc
{
    Texture::Texture(nl::node src)
    {
        if (src.data_type() == nl::node::type::bitmap)
        {
            // A source link shares pixels, not placement. The referencing
            // canvas keeps its own origin even when the artwork comes from a
            // different window (for example, NPC OK reuses a shop button).
            origin = src["origin"];
            std::string link = src["source"];
            if (!link.empty())
            {
                nl::node srcfile = src;
                while (srcfile != srcfile.root())
                {
                    srcfile = srcfile.root();
                }
                src = srcfile.resolve(link.substr(link.find('/') + 1));
            }

            bitmap = src;
            dimensions = Point<int16_t>(bitmap.width(),  bitmap.height());

            GraphicsGL::get().addbitmap(bitmap);
        }
    }

    Texture::Texture() {}

    Texture::~Texture() {}

    void Texture::draw(const DrawArgument& args) const
    {
        size_t id = bitmap.id();
        if (id == 0)
            return;

        const auto rect = args.get_rectangle(origin, dimensions);
        if (!args.get_color().invisible())
            DrawBounds::check(rect, DrawBounds::Kind::TEXTURE, id);
        GraphicsGL::get().draw(bitmap, rect, args.get_color(), args.get_angle());
    }

    void Texture::draw_clipped(const DrawArgument& args, Range<int16_t> vertical) const
    {
        if (bitmap.id() == 0) return;
        const auto rect = args.get_rectangle(origin, dimensions);
        const auto top = std::max(rect.t(), vertical.first());
        const auto bottom = std::min(rect.b(), vertical.second());
        if (top < bottom && !args.get_color().invisible())
            DrawBounds::check({rect.l(), rect.r(), top, bottom}, DrawBounds::Kind::TEXTURE, bitmap.id());
        GraphicsGL::get().draw_clipped(bitmap, rect, args.get_color(), vertical);
    }

    void Texture::shift(Point<int16_t> amount)
    {
        origin -= amount;
    }

    bool Texture::is_valid() const
    {
        return bitmap.id() > 0;
    }

    int16_t Texture::width() const
    {
        return dimensions.x();
    }

    int16_t Texture::height() const
    {
        return dimensions.y();
    }

    Point<int16_t> Texture::get_origin() const
    {
        return origin;
    }

    Point<int16_t> Texture::get_dimensions() const
    {
        return dimensions;
    }
}
