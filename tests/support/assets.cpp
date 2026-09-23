#include "assets.h"

#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>

namespace
{
    nl::file& open_asset(const std::string& name)
    {
        static std::map<std::string, std::unique_ptr<nl::file>> files;
        auto& file = files[name];
        if (!file)
        {
            const auto path = std::filesystem::path(TEST_ASSET_DIR) / name;
            if (!std::filesystem::is_regular_file(path))
                throw std::runtime_error("Missing read-only NX asset: " + path.string());
            file = std::make_unique<nl::file>(path.string());
        }
        return *file;
    }
}

namespace test_support
{
    NxFile::NxFile(const std::string& name) : file(open_asset(name)) {}

    NxFile::NxFile(const std::string& name, nl::node& target)
        : file(open_asset(name)), binding(&target), previous(target)
    {
        target = file.root();
    }

    NxFile::~NxFile()
    {
        if (binding)
            *binding = previous;
    }

    std::string artifact(const std::string& filename)
    {
        const std::filesystem::path directory("artifacts");
        std::filesystem::create_directories(directory);
        return (directory / filename).string();
    }
}
