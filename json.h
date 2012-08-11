#if !defined JSON_H
#define      JSON_H

#include <iosfwd>
#include <string>

class Track;

class JSON {
public:

    struct Options {
        std::string callback;
    };

    static void write(std::ostream & out,
                      Track & track,
                      Options option = Options());

};

#endif
