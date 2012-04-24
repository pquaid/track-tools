
#include <iostream>

class Track;

// Methods for generating a gnuplot script, creating an
// elevation profile

class Gnuplot {
public:

    struct Options {

        Options()
        : terminal("pngcairo size 800,300"), metric(false),
            grade(true), elevation(true), climbs(true), difficult(true)
            {}

        const char * terminal;
        bool metric;
        bool grade;
        bool elevation;
        bool climbs;
        bool difficult;
    };

    // Create a gnuplot script, based on the track
    static void write(std::ostream & out,
                      Track & track,
                      Options opt = Options());
};
