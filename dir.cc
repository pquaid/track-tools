#include "dir.h"
#include "exception.h"

#include <errno.h>

Directory::Directory(const char * dirname, bool skipDots) : skip(skipDots) {

    PRECONDITION(dirname != 0);

    directory = ::opendir(dirname);
    if (directory == 0) {
        throw SystemException(errno, "Opening directory");
    }
}

bool Directory::next() {

    while (true) {
        entry = readdir(directory);
        if (entry == 0) {
            return false;
        } else {
            name = entry->d_name;
            if (skip && ((name == "..")||(name == "."))) continue;
            return true;
        }
    }
}

std::string Directory::createPath(const std::string & path,
                                  const std::string & filename) {

    if (!path.empty() && (path[path.size()-1] != '/')) {
        return path + "/" + filename;
    } else {
        return path + filename;
    }
}
