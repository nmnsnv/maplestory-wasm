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

#include "node_impl.hpp"
#include "file_impl.hpp"
#include "bitmap.hpp"
#include "audio.hpp"
#include <cstring>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iostream>

#ifdef MS_PLATFORM_WASM
#include <LazyFS/LazyFileLoader.h>
#endif

namespace nl {
    // Helper accessors for LazyFS on WASM
#ifdef MS_PLATFORM_WASM
    
    // Access primitive type at offset
    template<typename T>
    inline const T* get_data(const _file_data* file_data, size_t offset) {
        if (!file_data || !file_data->base) return nullptr;
        auto loader = static_cast<LazyFS::LazyFileLoader*>(const_cast<void*>(file_data->base));
        return loader->get_data_at<T>(offset);
    }

    // Access contiguous array of T at offset
    template<typename T>
    inline const T* get_data_array(const _file_data* file_data, size_t offset, size_t count) {
        if (!file_data || !file_data->base) return nullptr;
        auto loader = static_cast<LazyFS::LazyFileLoader*>(const_cast<void*>(file_data->base));
        return static_cast<const T*>(loader->get_contiguous_data(offset, count * sizeof(T)));
    }

    // Access node::data using m_offset
    #define DATA_PTR (m_offset ? get_data<nl::node::data>(m_file, m_offset) : nullptr)
    
    // Helper to get string from string table index
    inline std::string get_string_from_table(const _file_data* file_data, uint32_t string_index) {
        if (!file_data) return {};
        // Get offset from string table
        size_t table_entry_offset = file_data->string_table + string_index * sizeof(uint64_t);
        auto offset_ptr = get_data<uint64_t>(file_data, table_entry_offset);
        if (!offset_ptr) return {};
        uint64_t string_offset = *offset_ptr;
        
        // Read string length (uint16_t)
        auto len_ptr = get_data<uint16_t>(file_data, string_offset);
        if (!len_ptr) return {};
        uint16_t len = *len_ptr;
        
        // Read string content (utf8 chars)
        auto char_ptr = get_data_array<char>(file_data, string_offset + 2, len);
        if (!char_ptr) return {};
        
        return {char_ptr, len};
    }
#else
    #define DATA_PTR m_data
#endif

    node::node(node const & o) :
#ifdef MS_PLATFORM_WASM
        m_offset(o.m_offset), m_file(o.m_file) {}
#else
        m_data(o.m_data), m_file(o.m_file) {}
#endif

#ifdef MS_PLATFORM_WASM
    // Constructor matching node.hpp signature but using offset internally? 
    // Wait, signature in node.hpp says (data const *, ...). 
    // We should change the constructor signature in node.hpp or assume 'd' is reused as offset on WASM?
    // Let's assume on WASM we pass offset masquerading as pointer, or change impl.
    // Actually, createLazyFile returns 'node' which calls this ctor.
    // Let's fix node::begin/end first.
    
    // NOTE: This constructor is used by node::begin() / node::end()
    // We need to ensure we pass something that makes sense.
    // Let's overload or change signature in .hpp if possible.
    // But modifying .hpp header signature breaks ABI/other files?
    // No, we already modified m_data/m_offset.
    
    // Let's pretend 'd' is actually the offset casted to pointer for now to minimize changes signature changes
    // But 'd' is 'data const *'.
    node::node(data const * d, file::data const * f) :
        m_offset(reinterpret_cast<uintptr_t>(d)), m_file(f) {} 
#else
    node::node(data const * d, file::data const * f) :
        m_data(d), m_file(f) {}
#endif

    node node::begin() const {
#ifdef MS_PLATFORM_WASM
        if (!m_offset)
            return {reinterpret_cast<data const*>(0), m_file};
        auto d = DATA_PTR;
        if (!d) return {reinterpret_cast<data const*>(0), m_file};
        
        uint64_t child_offset = m_file->node_table + d->children * sizeof(data);
        return {reinterpret_cast<data const*>(static_cast<uintptr_t>(child_offset)), m_file};
#else
        if (!m_data)
            return {nullptr, m_file};
        return {m_file->node_table + m_data->children, m_file};
#endif
    }
    node node::end() const {
#ifdef MS_PLATFORM_WASM
        if (!m_offset)
            return {reinterpret_cast<data const*>(0), m_file};
        auto d = DATA_PTR;
        if (!d) return {reinterpret_cast<data const*>(0), m_file};
        
        uint64_t end_offset = m_file->node_table + (d->children + d->num) * sizeof(data);
        return {reinterpret_cast<data const*>(static_cast<uintptr_t>(end_offset)), m_file};
#else
        if (!m_data)
            return {nullptr, m_file};
        return {m_file->node_table + m_data->children + m_data->num, m_file};
#endif
    }
    node node::operator*() const {
        return *this;
    }
    node & node::operator++() {
#ifdef MS_PLATFORM_WASM
        m_offset += sizeof(data);
#else
        ++m_data;
#endif
        return *this;
    }
    node node::operator++(int) {
#ifdef MS_PLATFORM_WASM
        auto temp = *this;
        m_offset += sizeof(data);
        return temp;
#else
        return {m_data++, m_file};
#endif
    }
    bool node::operator==(node const & o) const {
#ifdef MS_PLATFORM_WASM
        return m_offset == o.m_offset;
#else
        return m_data == o.m_data;
#endif
    }
    bool node::operator!=(node const & o) const {
#ifdef MS_PLATFORM_WASM
        return m_offset != o.m_offset;
#else
        return m_data != o.m_data;
#endif
    }
    bool node::operator<(node const & o) const {
#ifdef MS_PLATFORM_WASM
        return m_offset < o.m_offset;
#else
        return m_data < o.m_data;
#endif
    }
    std::string operator+(std::string s, node n) {
        return s + n.get_string();
    }
    std::string operator+(char const * s, node n) {
        return s + n.get_string();
    }
    std::string operator+(node n, std::string s) {
        return n.get_string() + s;
    }
    std::string operator+(node n, char const * s) {
        return n.get_string() + s;
    }
    node node::operator[](unsigned int n) const {
        return operator[](std::to_string(n));
    }
    node node::operator[](signed int n) const {
        return operator[](std::to_string(n));
    }
    node node::operator[](unsigned long n) const {
        return operator[](std::to_string(n));
    }
    node node::operator[](signed long n) const {
        return operator[](std::to_string(n));
    }
    node node::operator[](unsigned long long n) const {
        return operator[](std::to_string(n));
    }
    node node::operator[](signed long long n) const {
        return operator[](std::to_string(n));
    }
    node node::operator[](std::string const & o) const {
        return get_child(o.c_str(), static_cast<uint16_t>(o.length()));
    }
    node node::operator[](char const * o) const {
        return get_child(o, static_cast<uint16_t>(std::strlen(o)));
    }
    node node::operator[](node const & o) const {
        return operator[](o.get_string());
    }
    node::operator unsigned char() const {
        return static_cast<unsigned char>(get_integer());
    }
    node::operator signed char() const {
        return static_cast<signed char>(get_integer());
    }
    node::operator unsigned short() const {
        return static_cast<unsigned short>(get_integer());
    }
    node::operator signed short() const {
        return static_cast<signed short>(get_integer());
    }
    node::operator unsigned int() const {
        return static_cast<unsigned int>(get_integer());
    }
    node::operator signed int() const {
        return static_cast<signed int>(get_integer());
    }
    node::operator unsigned long() const {
        return static_cast<unsigned long>(get_integer());
    }
    node::operator signed long() const {
        return static_cast<signed long>(get_integer());
    }
    node::operator unsigned long long() const {
        return static_cast<unsigned long long>(get_integer());
    }
    node::operator signed long long() const {
        return static_cast<signed long long>(get_integer());
    }
    node::operator float() const {
        return static_cast<float>(get_real());
    }
    node::operator double() const {
        return static_cast<double>(get_real());
    }
    node::operator long double() const {
        return static_cast<long double>(get_real());
    }
    node::operator std::string() const {
        return get_string();
    }
    node::operator vector2i() const {
        return get_vector();
    }
    node::operator bitmap() const {
        return get_bitmap();
    }
    node::operator audio() const {
        return get_audio();
    }
    node::operator bool() const {
#ifdef MS_PLATFORM_WASM
        return m_offset != 0;
#else
        return m_data ? true : false;
#endif
    }
    int64_t node::get_integer(int64_t def) const {
        auto d = DATA_PTR;
        if (!d)
            return def;
        switch (d->type) {
        case type::none:
        case type::vector:
        case type::bitmap:
        case type::audio:
            return def;
        case type::integer:
            return to_integer();
        case type::real:
            return static_cast<int64_t>(to_real());
        case type::string:
            return std::stoll(to_string());
        default:
            throw std::runtime_error("Unknown node type");
        }
    }
    double node::get_real(double def) const {
        auto d = DATA_PTR;
        if (!d)
            return def;
        switch (d->type) {
        case type::none:
        case type::vector:
        case type::bitmap:
        case type::audio:
            return def;
        case type::integer:
            return static_cast<double>(to_integer());
        case type::real:
            return to_real();
        case type::string:
            return std::stod(to_string());
        default:
            throw std::runtime_error("Unknown node type");
        }
    }
    std::string node::get_string(std::string def) const {
        auto d = DATA_PTR;
        if (!d)
            return def;
        switch (d->type) {
        case type::none:
        case type::vector:
        case type::bitmap:
        case type::audio:
            return def;
        case type::integer:
            return std::to_string(to_integer());
        case type::real:
            return std::to_string(to_real());
        case type::string:
            return to_string();
        default:
            throw std::runtime_error("Unknown node type");
        }
    }
    vector2i node::get_vector(vector2i def) const {
        auto d = DATA_PTR;
        if (d && d->type == type::vector)
            return to_vector();
        return def;
    }
    bitmap node::get_bitmap() const {
#ifdef MS_PLATFORM_WASM
        auto d = DATA_PTR;
        // Note: Some NX files report bitmap_count=0 in their header but still
        // have valid bitmap nodes. On WASM we skip the bitmap_count check and
        // trust the node type, since to_bitmap() resolves data via LazyFS.
        if (d && d->type == type::bitmap)
            return to_bitmap();
        return {nullptr, 0, 0, 0};
#else
        if (m_data && m_data->type == type::bitmap && m_file->header->bitmap_count)
            return to_bitmap();
        return {nullptr, 0, 0, 0};
#endif
    }
    audio node::get_audio() const {
#ifdef MS_PLATFORM_WASM
        auto d = DATA_PTR;
        auto header_ptr = get_data<file::header>(m_file, m_file->header);
        if (d && d->type == type::audio && header_ptr && header_ptr->audio_count)
            return to_audio();
        return {nullptr, 0};
#else
        if (m_data && m_data->type == type::audio && m_file->header->audio_count)
            return to_audio();
        return {nullptr, 0};
#endif
    }
    bool node::get_bool() const {
        auto d = DATA_PTR;
        return d && d->type == type::integer && to_integer() ? true : false;
    }
    bool node::get_bool(bool def) const {
        auto d = DATA_PTR;
        return d && d->type == type::integer ? to_integer() ? true : false : def;
    }
    int32_t node::x() const {
        auto d = DATA_PTR;
        return d && d->type == type::vector ? d->vector[0] : 0;
    }
    int32_t node::y() const {
        auto d = DATA_PTR;
        return d && d->type == type::vector ? d->vector[1] : 0;
    }
    std::string node::name() const {
        auto d = DATA_PTR;
        if (!d)
            return {};
#ifdef MS_PLATFORM_WASM
        return get_string_from_table(m_file, d->name);
#else
        auto const s = reinterpret_cast<char const *>(m_file->base)
            + m_file->string_table[m_data->name];
        return {s + 2, *reinterpret_cast<uint16_t const *>(s)};
#endif
    }
    size_t node::size() const {
        auto d = DATA_PTR;
        return d ? d->num : 0u;
    }
    node::type node::data_type() const {
        auto d = DATA_PTR;
        return d ? d->type : type::none;
    }
    node node::get_child(char const * const o, uint16_t const l) const {
        auto d = DATA_PTR;
        if (!d)
            return {reinterpret_cast<data const*>(0), m_file};

#ifdef MS_PLATFORM_WASM
        // WASM Implementation using offsets
        auto n = d->num;
        // Start offset of child nodes
        auto p_offset = m_file->node_table + d->children * sizeof(data);
        
        for (;;) {
            if (!n)
                return {reinterpret_cast<data const*>(0), m_file};
            
            auto const n2 = static_cast<decltype(n)>(n >> 1);
            auto const p2_offset = p_offset + n2 * sizeof(data);
            
            auto p2 = get_data<data>(m_file, p2_offset);
            if (!p2) return {reinterpret_cast<data const*>(0), m_file};

            // Resolve name from string table
            // Read string table offset
            size_t string_table_entry = m_file->string_table + p2->name * sizeof(uint64_t);
            auto str_offset_ptr = get_data<uint64_t>(m_file, string_table_entry);
            if (!str_offset_ptr)
                return {reinterpret_cast<data const*>(0), m_file};
            
            // Read string length and content
            uint64_t name_offset = *str_offset_ptr;
            auto name_len_ptr = get_data<uint16_t>(m_file, name_offset);
            if (!name_len_ptr)
                return {reinterpret_cast<data const*>(0), m_file};
            
            auto const l1 = *name_len_ptr;
            // Use get_data_array because valid length check is needed
            auto name_chars_ptr = get_data_array<uint8_t>(m_file, name_offset + 2, l1);
            if (!name_chars_ptr)
                return {reinterpret_cast<data const*>(0), m_file};
            auto const s = name_chars_ptr;
            auto const os = reinterpret_cast<uint8_t const *>(o);

            bool z = false;
            auto const len = l1 < l ? l1 : l;
            for (auto i = 0U; i < len; ++i) {
                if (s[i] > os[i]) {
                    n = n2;
                    z = true;
                    break;
                } else if (s[i] < os[i]) {
                    p_offset = p2_offset + sizeof(data);
                    n -= n2 + 1;
                    z = true;
                    break;
                }
            }
            if (z)
                continue;
            else if (l1 < l)
                p_offset = p2_offset + sizeof(data), n -= n2 + 1;
            else if (l1 > l)
                n = n2;
            else
                return {reinterpret_cast<data const*>(static_cast<uintptr_t>(p2_offset)), m_file};
        }
#else
        auto p = m_file->node_table + m_data->children;
        auto n = m_data->num;
        auto const b = reinterpret_cast<const char *>(m_file->base);
        auto const t = m_file->string_table;
        for (;;) {
            if (!n)
                return {nullptr, m_file};
            auto const n2 = static_cast<decltype(n)>(n >> 1);
            auto const p2 = p + n2;
            auto const sl = b + t[p2->name];
            auto const l1 = *reinterpret_cast<uint16_t const *>(sl);
            auto const s = reinterpret_cast<uint8_t const *>(sl + 2);
            auto const os = reinterpret_cast<uint8_t const *>(o);
            bool z = false;
            auto const len = l1 < l ? l1 : l;
            for (auto i = 0U; i < len; ++i) {
                if (s[i] > os[i]) {
                    n = n2;
                    z = true;
                    break;
                } else if (s[i] < os[i]) {
                    p = p2 + 1;
                    n -= n2 + 1;
                    z = true;
                    break;
                }
            }
            if (z)
                continue;
            else if (l1 < l)
                p = p2 + 1, n -= n2 + 1;
            else if (l1 > l)
                n = n2;
            else
                return {p2, m_file};
        }
#endif
    }
    int64_t node::to_integer() const {
        auto d = DATA_PTR;
        return d ? d->ireal : 0;
    }
    double node::to_real() const {
        auto d = DATA_PTR;
        return d ? d->dreal : 0.0;
    }
    std::string node::to_string() const {
        auto d = DATA_PTR;
        if (!d) return {};
#ifdef MS_PLATFORM_WASM
        return get_string_from_table(m_file, d->string);
#else
        auto const s = reinterpret_cast<char const *>(m_file->base)
            + m_file->string_table[m_data->string];
        return {s + 2, *reinterpret_cast<uint16_t const *>(s)};
#endif
    }
    vector2i node::to_vector() const {
        auto d = DATA_PTR;
        return d ? vector2i{d->vector[0], d->vector[1]} : vector2i{0,0};
    }
    bitmap node::to_bitmap() const {
        auto d = DATA_PTR;
        if (!d) return {nullptr, 0, 0, 0};
#ifdef MS_PLATFORM_WASM
        size_t bm_table_entry = m_file->bitmap_table + d->bitmap.index * sizeof(uint64_t);
        auto offset_ptr = get_data<uint64_t>(m_file, bm_table_entry);
        uint64_t bitmap_offset = offset_ptr ? *offset_ptr : 0;
        
        // Pass file handle and offset to bitmap - it will use io_read to fetch data
        return {const_cast<file::data*>(m_file), bitmap_offset, d->bitmap.width, d->bitmap.height};
#else
        return {const_cast<file::data*>(m_file), m_file->bitmap_table[m_data->bitmap.index],
            m_data->bitmap.width, m_data->bitmap.height};
#endif
    }
    audio node::to_audio() const {
        auto d = DATA_PTR;
        if (!d) return {nullptr, 0};
#ifdef MS_PLATFORM_WASM
        size_t au_table_entry = m_file->audio_table + d->audio.index * sizeof(uint64_t);
        auto offset_ptr = get_data<uint64_t>(m_file, au_table_entry);
        uint64_t audio_offset = offset_ptr ? *offset_ptr : 0;
        
        auto data_ptr = get_data_array<char>(m_file, audio_offset, d->audio.length);
        return {data_ptr, d->audio.length};
#else
        return {reinterpret_cast<char const *>(m_file->base)
            + m_file->audio_table[m_data->audio.index],
            m_data->audio.length};
#endif
    }
    node node::root() const {
#ifdef MS_PLATFORM_WASM
        // m_file->node_table is offset of root node
         return {reinterpret_cast<data const*>(static_cast<uintptr_t>(m_file->node_table)), m_file};
#else
        return {m_file->node_table, m_file};
#endif
    }
    node node::resolve(std::string path) const {
        std::istringstream stream(path);
        std::vector<std::string> parts;
        std::string segment;
        while (std::getline(stream, segment, '/'))
            parts.push_back(segment);
        auto n = *this;
        for (auto & part : parts) {
            n = n[part];
        }
        return n;
    }
}
