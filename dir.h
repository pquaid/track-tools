#if !defined DIR_H
#define      DIR_H

#include <dirent.h>
#include <string>


class Directory {
public:
    Directory(const char * dir, bool skipDots = true);

    bool next();

    // Return the filename, only (ie no directory). This is only
    // valid after a successful call to next().
    const std::string & getFilename() const { return name; }

    static std::string createPath(const std::string & directory,
                                  const std::string & filename);

    static std::string basename(const std::string & path);

private:

    DIR * directory;
    dirent * entry;

    std::string name;
    bool skip;
};

#endif
