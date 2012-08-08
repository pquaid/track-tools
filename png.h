#if !defined PNG_H
#define      PNG_H

#include <iosfwd>
#include <string>

class Track;

class PNG {
public:

    struct Options {

        Options() : width(800), height(300), metric(false),
            grade(true), climbs(true), difficult(true),
            pointSize(10), font("helvetica")
            {}

        int width;
        int height;
        bool metric;
        bool grade;
        bool climbs;
        bool difficult;
        double pointSize;
        std::string font;
    };

    static void write(std::ostream & out,
                      Track & track,
                      Options opt = Options());
};

#endif
