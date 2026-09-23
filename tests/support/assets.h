#pragma once

#include "nlnx/file.hpp"
#include "nlnx/node.hpp"

#include <string>

namespace test_support
{
    // Files stay open until process exit because production metadata caches
    // retain NX nodes. A fixture restores only the root it temporarily binds.
    class NxFile
    {
    public:
        explicit NxFile(const std::string& name);
        NxFile(const std::string& name, nl::node& binding);
        ~NxFile();
        NxFile(const NxFile&) = delete;
        NxFile& operator=(const NxFile&) = delete;
        nl::node root() const { return file.root(); }

    private:
        nl::file& file;
        nl::node* binding = nullptr;
        nl::node previous;
    };

    // Diagnostics live below the executable's CTest working directory.
    std::string artifact(const std::string& filename);
}
