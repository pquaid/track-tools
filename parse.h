#if !defined PARSE_H
#define      PARSE_H

#include <string>

class Track;

class Parse {
public:

    enum Format {
        FORMAT_GPX,
        FORMAT_KML,
        FORMAT_FIT,
        FORMAT_TEXT,
        FORMAT_PNG,
        FORMAT_JSON,
        FORMAT_UNKNOWN
    };

    static void read(Track & track, const std::string & filename,
                     Format format = FORMAT_UNKNOWN);

    static Format stringToFormat(const std::string & format);
};

#endif
