#if !defined KML_H
#define      KML_H

#include <vector>
#include <string>
#include <exception>
#include <iosfwd>

#include <stdint.h>

#include "exception.h"
#include "track.h"


// Methods for reading and writing KML files
class KML {
public:
  struct Options {
    Options() {}

    bool startAndEnd = true;   // add start/end icons
    double width = 2.0;        // width of the line
    uint32_t color = 0xbb0000; // color, as bgr (eg 0x0000ff == red)
    uint32_t opacity = 0xff;   // alpha of line: 0 (transparent) to 255 (opaque)
  };

  static void read(std::istream& in, Track& out);
  static void read(const std::string& filename, Track& out);

  static void write(std::ostream& out,
                    const Track& track,
                    Options options = Options());
};

class KMLError : public Exception {
public:
  KMLError(const std::string& msg)
      : Exception("Error parsing KML: " + msg)
  {}
};

#endif
