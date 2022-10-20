#ifndef FILE_SYSTEM_INCLUDED
#define FILE_SYSTEM_INCLUDED

#include "../My/String.hpp"

#define GLOBAL_FILEPATH_SIZE 1024

/*
namespace FilePaths {
    // Base path to resources
    extern char base[GLOBAL_FILEPATH_SIZE];
    extern char assets[GLOBAL_FILEPATH_SIZE];
    extern char shaders[GLOBAL_FILEPATH_SIZE]; // path to shaders
    extern char save[GLOBAL_FILEPATH_SIZE]; // path to save folder
}
*/

struct FileSystemT {

    struct Directory {
        char* path;

        Directory() {}

        Directory(const char* unixPath) {
            path = My::strdup(unixPath);
        }

        Directory(Directory root, const char* subpath) {
            path = My::str_add_alloc(root.path, subpath);
        }

        My::CString get(const char* unixFilepath) const {
            // make this work for windows
            return My::str_add(path, FileSystemT::get(unixFilepath));
        }

        const char* get() const {
            return path;
        }
    };

    Directory resources;
    Directory assets;
    Directory shaders;
    Directory save;

    FileSystemT() {}

    FileSystemT(const char* unixResourcesPath) {
        resources = Directory(get(unixResourcesPath));
        assets = Directory(get(resources, "assets/"));
        shaders = Directory(get(resources, "shaders/"));
        save = Directory(get(resources, "save/"));
    }

    static My::CString get(const char* unixFilepath) {
        return My::CString(unixFilepath);
    }

    static My::CString get(Directory directory, const char* unixFilepath) {
        return My::str_add(directory.path, get(unixFilepath));
    }

    void destroy() {
        free(resources.path);
        free(assets.path);
        free(shaders.path);
        free(save.path);
    }
};

extern FileSystemT FileSystem;

#endif