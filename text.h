#if !defined TEXT_H
#define      TEXT_H

#include <iosfwd>

class Track;

// A simple format, mainly for testing.
//
//  * Lines starting with '#' are ignored
//  * Lines starting with @ are points
//  * Lines starting with letters are 'name=value'
//
// The values are just space-separated, in sequence

class Text {
public:
  static void read(std::istream & in, Track & track);
  static void write(std::ostream & out, const Track & track);
};

#endif
