#if !defined JS_H
#define      JS_H

#include <iosfwd>
#include <string>

class Track;

class JS {
public:

    static void write(std::ostream & out, Track & track);

};

#endif
