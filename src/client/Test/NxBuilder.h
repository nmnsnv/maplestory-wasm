// Test-only builder that serializes a synthetic NX (PKG4) image in memory.
//
// nl::node is a read-only view over a memory-mapped PKG4 file and cannot be
// constructed with data through its public API, so to build synthetic assets in
// code (Plan 07, Phase D) we emit a real PKG4 byte image and load it via
// nl::file::open_memory(). This lets tests construct real UI windows against
// fully in-code assets, with no committed binary fixture.
//
// Supported value types: none (branch nodes), integer, string, vector, and
// bitmap (width/height + zeroed, LZ4-compressed pixels). Bitmaps should be at
// least a few pixels so their compressed size stays below the raw size.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace jrc::test
{
    class NxBuilder
    {
    public:
        // Opaque handle to a node within the builder.
        using Node = std::size_t;

        NxBuilder();

        // The root node (index 0). Give children to it to form the tree.
        Node root() const;

        // Add a named child (a branch node by default) and return its handle.
        Node add_child(Node parent, const std::string& name);

        // Assign a value to a node. A node carries at most one value type.
        void set_integer(Node node, int64_t value);
        void set_string(Node node, const std::string& value);
        void set_vector(Node node, int32_t x, int32_t y);
        void set_bitmap(Node node, uint16_t width, uint16_t height);

        // Convenience: add a child and set its value in one call.
        Node add_integer(Node parent, const std::string& name, int64_t value);
        Node add_string(Node parent, const std::string& name, const std::string& value);
        Node add_vector(Node parent, const std::string& name, int32_t x, int32_t y);
        Node add_bitmap(Node parent, const std::string& name, uint16_t width, uint16_t height);

        // Serialize the tree into a self-contained PKG4 byte image. The returned
        // buffer must outlive any nl::file opened over it.
        std::vector<uint8_t> build() const;

    private:
        enum class Type : uint16_t
        {
            none = 0,
            integer = 1,
            real = 2,
            string = 3,
            vector = 4,
            bitmap = 5,
            audio = 6,
        };

        struct NodeDef
        {
            std::string name;
            Type type = Type::none;
            int64_t ivalue = 0;
            int32_t vec[2] = {0, 0};
            uint16_t bw = 0;
            uint16_t bh = 0;
            std::string svalue;
            std::vector<std::size_t> children;
        };

        std::vector<NodeDef> nodes;
    };
}
