#ifndef ALOHA_UTILS_PATHS_H
#define ALOHA_UTILS_PATHS_H

#include <string>
#include <filesystem>

namespace aloha::utils
{

    struct StdlibPaths
    {
        std::filesystem::path root;
        std::filesystem::path source_dir;
        std::filesystem::path library_file;
    };

    std::filesystem::path get_aloha_root();

    StdlibPaths get_stdlib_paths();

    std::string get_stdlib_archive();

} // namespace aloha::utils

#endif // ALOHA_UTILS_PATHS_H
