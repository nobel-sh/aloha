#include "paths.h"
#include <cstdlib>

#if defined(__linux__)
#include <linux/limits.h>
#include <unistd.h>
#endif

namespace aloha::utils
{

    std::filesystem::path get_aloha_root()
    {
        // used in development environment for overriding stdlib location
        if (const char *aloha_dev = std::getenv("ALOHA_DEV"))
        {
            std::filesystem::path root_path(aloha_dev);
            if (std::filesystem::exists(root_path / "stdlib"))
            {
                return root_path;
            }
        }

#ifdef ALOHA_ROOT
        std::filesystem::path aloha_root(ALOHA_ROOT);
        if (std::filesystem::exists(aloha_root / "stdlib"))
        {
            return aloha_root;
        }
#endif

        // Default fallback to ~/.aloha
        if (const char *home = std::getenv("HOME"))
        {
            std::filesystem::path fallback_dir = std::filesystem::path(home) / ".aloha";
            if (std::filesystem::exists(fallback_dir / "stdlib"))
            {
                return fallback_dir;
            }
        }

        return "";
    }

    std::string get_stdlib_archive()
    {
        return get_stdlib_paths().library_file.string();
    }

    StdlibPaths get_stdlib_paths()
    {
        StdlibPaths paths;
        paths.root = get_aloha_root();

        if (!paths.root.empty())
        {
            paths.source_dir = paths.root / "stdlib";

            std::filesystem::path build_lib = paths.root / "build" / "libaloha_stdlib.a";
            if (std::filesystem::exists(build_lib))
            {
                paths.library_file = build_lib;
                return paths;
            }

            std::filesystem::path install_lib = paths.root / "lib" / "libaloha_stdlib.a";
            if (std::filesystem::exists(install_lib))
            {
                paths.library_file = install_lib;
                return paths;
            }
        }

#if defined(__linux__)
        // Fallback: try to find relative to compiler executable
        char exe_path[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len != -1)
        {
            exe_path[len] = '\0';
            std::filesystem::path exe_dir = std::filesystem::path(exe_path).parent_path();
            std::filesystem::path stdlib_path = exe_dir / "libaloha_stdlib.a";
            if (std::filesystem::exists(stdlib_path))
            {
                paths.library_file = stdlib_path;
                return paths;
            }
        }
#endif

        // last resort fallback
        paths.library_file = "../build/libaloha_stdlib.a";
        return paths;
    }

} // namespace aloha::utils
