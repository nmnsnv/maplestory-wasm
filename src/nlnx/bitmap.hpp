//////////////////////////////////////////////////////////////////////////////
// NoLifeNx - Part of the NoLifeStory project                               //
// Copyright © 2013 Peter Atashian                                          //
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

#pragma once
#include "nxfwd.hpp"
#include <cstddef>
#include <cstdint>

namespace nl
{
	class bitmap
	{
	  public:
		bitmap() : m_file(nullptr), m_offset(0), m_width(0), m_height(0)
		{
		}
		bitmap(void* f, uint64_t o, uint16_t w, uint16_t h);
		bool operator<(bitmap const&) const;
		bool operator==(bitmap const&) const;
		operator bool() const;
		void const* data() const;
		uint16_t width() const;
		uint16_t height() const;
		uint32_t length() const;
		size_t id() const;

	  private:
		void* m_file;
		uint64_t m_offset;
		uint16_t m_width;
		uint16_t m_height;
	};
}
