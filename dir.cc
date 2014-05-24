#include "dir.h"
#include "exception.h"

#include <string>

#include <errno.h>

using namespace std;

Directory::Directory(const char* dirname, const bool skipDots)
    : skip(skipDots) {
  PRECONDITION(dirname != 0);

  directory = ::opendir(dirname);
  if (directory == 0) {
    throw SystemException(errno, "Opening directory");
  }
}

bool Directory::next() {
  while (true) {
    entry = readdir(directory);
    if (entry == nullptr) {
      return false;
    } else {
      name = entry->d_name;
      if (skip && (name == ".." || name == ".")) continue;
      return true;
    }
  }
}

std::string Directory::createPath(const std::string& path,
                                  const std::string& filename) {
  if (!path.empty() && (path[path.size()-1] != '/')) {
    return path + "/" + filename;
  } else {
    return path + filename;
  }
}


string Directory::basename(const string& str) {
  const string::size_type p = str.find_last_of('/');
  if (p != string::npos && p < (str.size()-1)) {
    return str.substr(p+1);
  } else {
    return str;
  }
}
