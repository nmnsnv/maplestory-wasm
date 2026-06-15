#include "NxBuilder.h"

#include <lz4.h>

#include <algorithm>
#include <cstring>
#include <map>

namespace jrc::test
{
    namespace
    {
        // Append a little-endian integer to a byte buffer. The PKG4 reader uses
        // reinterpret_cast over packed structs, so on the (little-endian) host
        // native byte order is what it expects.
        template <typename T>
        void put(std::vector<uint8_t>& buf, T value)
        {
            uint8_t bytes[sizeof(T)];
            std::memcpy(bytes, &value, sizeof(T));
            buf.insert(buf.end(), bytes, bytes + sizeof(T));
        }
    }

    NxBuilder::NxBuilder()
    {
        // Node 0 is the (unnamed) root.
        nodes.push_back(NodeDef{});
    }

    NxBuilder::Node NxBuilder::root() const
    {
        return 0;
    }

    NxBuilder::Node NxBuilder::add_child(Node parent, const std::string& name)
    {
        Node id = nodes.size();
        NodeDef def;
        def.name = name;
        nodes.push_back(def);
        nodes[parent].children.push_back(id);
        return id;
    }

    void NxBuilder::set_integer(Node node, int64_t value)
    {
        nodes[node].type = Type::integer;
        nodes[node].ivalue = value;
    }

    void NxBuilder::set_string(Node node, const std::string& value)
    {
        nodes[node].type = Type::string;
        nodes[node].svalue = value;
    }

    void NxBuilder::set_vector(Node node, int32_t x, int32_t y)
    {
        nodes[node].type = Type::vector;
        nodes[node].vec[0] = x;
        nodes[node].vec[1] = y;
    }

    void NxBuilder::set_bitmap(Node node, uint16_t width, uint16_t height)
    {
        nodes[node].type = Type::bitmap;
        nodes[node].bw = width;
        nodes[node].bh = height;
    }

    NxBuilder::Node NxBuilder::add_integer(Node parent, const std::string& name, int64_t value)
    {
        Node id = add_child(parent, name);
        set_integer(id, value);
        return id;
    }

    NxBuilder::Node NxBuilder::add_string(Node parent, const std::string& name, const std::string& value)
    {
        Node id = add_child(parent, name);
        set_string(id, value);
        return id;
    }

    NxBuilder::Node NxBuilder::add_vector(Node parent, const std::string& name, int32_t x, int32_t y)
    {
        Node id = add_child(parent, name);
        set_vector(id, x, y);
        return id;
    }

    NxBuilder::Node NxBuilder::add_bitmap(Node parent, const std::string& name, uint16_t width, uint16_t height)
    {
        Node id = add_child(parent, name);
        set_bitmap(id, width, height);
        return id;
    }

    std::vector<uint8_t> NxBuilder::build() const
    {
        const size_t n = nodes.size();

        // 1. Lay out nodes breadth-first so each node's children form a single
        //    contiguous, name-sorted block (required by node::get_child's binary
        //    search). order[i] is the node id stored at table index i.
        std::vector<size_t> order{0};
        std::vector<uint32_t> child_start(n, 0);
        std::vector<uint16_t> child_num(n, 0);
        std::vector<std::vector<size_t>> sorted_children(n);

        for (size_t i = 0; i < order.size(); ++i)
        {
            size_t id = order[i];
            std::vector<size_t> kids = nodes[id].children;
            std::sort(kids.begin(), kids.end(), [&](size_t a, size_t b) {
                return nodes[a].name < nodes[b].name;
            });
            sorted_children[id] = kids;
            child_start[id] = static_cast<uint32_t>(order.size());
            child_num[id] = static_cast<uint16_t>(kids.size());
            for (size_t kid : kids)
            {
                order.push_back(kid);
            }
        }

        // 2. Intern strings (node names + string values).
        std::vector<std::string> strings;
        std::map<std::string, uint32_t> string_index;
        auto intern = [&](const std::string& s) {
            auto it = string_index.find(s);
            if (it != string_index.end())
            {
                return it->second;
            }
            uint32_t idx = static_cast<uint32_t>(strings.size());
            strings.push_back(s);
            string_index.emplace(s, idx);
            return idx;
        };
        std::vector<uint32_t> name_idx(n);
        std::vector<uint32_t> strval_idx(n, 0);
        for (size_t i = 0; i < n; ++i)
        {
            name_idx[i] = intern(nodes[i].name);
            if (nodes[i].type == Type::string)
            {
                strval_idx[i] = intern(nodes[i].svalue);
            }
        }

        // 3. Assign bitmap indices.
        std::vector<uint32_t> bmp_idx(n, 0);
        std::vector<size_t> bitmap_nodes;
        for (size_t i = 0; i < n; ++i)
        {
            if (nodes[i].type == Type::bitmap)
            {
                bmp_idx[i] = static_cast<uint32_t>(bitmap_nodes.size());
                bitmap_nodes.push_back(i);
            }
        }

        const uint32_t node_count = static_cast<uint32_t>(order.size());
        const uint32_t string_count = static_cast<uint32_t>(strings.size());
        const uint32_t bitmap_count = static_cast<uint32_t>(bitmap_nodes.size());

        // 4. Compute section offsets. Layout:
        //    header | node table | string table | bitmap table | strings | bitmaps
        constexpr uint64_t HEADER_SIZE = 52; // packed file::header
        constexpr uint64_t NODE_SIZE = 20;   // packed node::data
        const uint64_t node_table_off = HEADER_SIZE;
        const uint64_t string_table_off = node_table_off + NODE_SIZE * node_count;
        const uint64_t bitmap_table_off = string_table_off + 8ull * string_count;
        const uint64_t string_blob_off = bitmap_table_off + 8ull * bitmap_count;

        std::vector<uint64_t> string_offset(string_count);
        uint64_t cursor = string_blob_off;
        for (uint32_t i = 0; i < string_count; ++i)
        {
            string_offset[i] = cursor;
            cursor += 2 + strings[i].size(); // [u16 len][bytes]
        }

        // Pre-compress each bitmap's (zeroed) pixels. The reader fetches the raw
        // size (4*w*h) bytes starting 4 bytes into the entry and LZ4-decodes them.
        std::vector<uint64_t> bitmap_offset(bitmap_count);
        std::vector<std::vector<char>> bitmap_compressed(bitmap_count);
        std::vector<uint32_t> bitmap_raw_len(bitmap_count);
        for (uint32_t i = 0; i < bitmap_count; ++i)
        {
            const NodeDef& nd = nodes[bitmap_nodes[i]];
            uint32_t raw_len = 4u * nd.bw * nd.bh;
            bitmap_raw_len[i] = raw_len;

            std::vector<char> src(raw_len, 0);
            int bound = LZ4_compressBound(static_cast<int>(raw_len));
            std::vector<char> dst(static_cast<size_t>(bound));
            int csize = LZ4_compress_default(src.data(), dst.data(),
                                             static_cast<int>(raw_len), bound);
            dst.resize(static_cast<size_t>(csize));
            bitmap_compressed[i] = std::move(dst);

            bitmap_offset[i] = cursor;
            // 4-byte prefix + a raw_len-sized region holding the compressed
            // stream (padded with zeroes); the reader reads raw_len bytes here.
            cursor += 4 + raw_len;
        }

        const uint64_t total_size = cursor;

        // 5. Emit the image. Sections are appended in offset order.
        std::vector<uint8_t> buf;
        buf.reserve(total_size);

        // Header.
        put<uint32_t>(buf, 0x34474B50);          // magic "PKG4"
        put<uint32_t>(buf, node_count);
        put<uint64_t>(buf, node_table_off);
        put<uint32_t>(buf, string_count);
        put<uint64_t>(buf, string_offset.empty() ? string_blob_off : string_table_off);
        put<uint32_t>(buf, bitmap_count);
        put<uint64_t>(buf, bitmap_table_off);
        put<uint32_t>(buf, 0);                    // audio_count
        put<uint64_t>(buf, 0);                    // audio_offset

        // Node table.
        for (size_t i = 0; i < order.size(); ++i)
        {
            size_t id = order[i];
            const NodeDef& nd = nodes[id];
            put<uint32_t>(buf, name_idx[id]);
            put<uint32_t>(buf, child_start[id]);
            put<uint16_t>(buf, child_num[id]);
            put<uint16_t>(buf, static_cast<uint16_t>(nd.type));

            // 8-byte value union.
            switch (nd.type)
            {
            case Type::integer:
                put<int64_t>(buf, nd.ivalue);
                break;
            case Type::string:
                put<uint32_t>(buf, strval_idx[id]);
                put<uint32_t>(buf, 0); // padding to fill the union
                break;
            case Type::vector:
                put<int32_t>(buf, nd.vec[0]);
                put<int32_t>(buf, nd.vec[1]);
                break;
            case Type::bitmap:
                put<uint32_t>(buf, bmp_idx[id]);
                put<uint16_t>(buf, nd.bw);
                put<uint16_t>(buf, nd.bh);
                break;
            default:
                put<uint64_t>(buf, 0);
                break;
            }
        }

        // String table (absolute offsets).
        for (uint32_t i = 0; i < string_count; ++i)
        {
            put<uint64_t>(buf, string_offset[i]);
        }

        // Bitmap table (absolute offsets).
        for (uint32_t i = 0; i < bitmap_count; ++i)
        {
            put<uint64_t>(buf, bitmap_offset[i]);
        }

        // String blob.
        for (uint32_t i = 0; i < string_count; ++i)
        {
            put<uint16_t>(buf, static_cast<uint16_t>(strings[i].size()));
            buf.insert(buf.end(), strings[i].begin(), strings[i].end());
        }

        // Bitmap blob: [u32 compressed-size][compressed bytes][zero pad to raw_len].
        for (uint32_t i = 0; i < bitmap_count; ++i)
        {
            const std::vector<char>& comp = bitmap_compressed[i];
            put<uint32_t>(buf, static_cast<uint32_t>(comp.size()));
            buf.insert(buf.end(), comp.begin(), comp.end());
            // Pad the raw_len-sized data region.
            for (uint32_t pad = static_cast<uint32_t>(comp.size()); pad < bitmap_raw_len[i]; ++pad)
            {
                buf.push_back(0);
            }
        }

        return buf;
    }
}
