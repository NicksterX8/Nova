#ifndef FILE_SYSTEM_INCLUDED
#define FILE_SYSTEM_INCLUDED

#include "My/String.hpp"

#define GLOBAL_FILEPATH_SIZE 1024

namespace My {
    using String = AllocatedString;
};

namespace FilePaths {
    // Base path to resources
    extern char base[GLOBAL_FILEPATH_SIZE];
    extern char assets[GLOBAL_FILEPATH_SIZE];
    extern char shaders[GLOBAL_FILEPATH_SIZE]; // path to shaders
    extern char save[GLOBAL_FILEPATH_SIZE]; // path to save folder
}

struct FileSystemT {
    struct Directory {
        const char* path;

        Directory() {
            path = nullptr;
        }

        Directory(const char* unixFilepath) {
            path = unixFilepath;
        }

        My::String get(const char* unixFilepath) const {
            // make this work for windows
            return str_add(path, unixFilepath);
        }
    };

    Directory resources;
    Directory assets;
    Directory shaders;

    FileSystemT(const char* unixResourcesPath) {
        resources = Directory(get(unixResourcesPath));
    }

    My::String get(const char* unixFilepath) const {
        return unixFilepath;
    }

    My::String get(Directory directory, const char* unixFilepath) const {
        return str_add(directory.path, unixFilepath);
    }
};

extern FileSystemT FileSystem;

#endif