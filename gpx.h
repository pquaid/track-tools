#if !defined GPX_H
#define      GPX_H

#include "exception.h"

#include <iosfwd>
#include <string>

class Track;

// Methods for reading and writing GPX files
class GPX {
public:
  static void read(std::istream& in, Track& out);
  static void read(const std::string& filename, Track& out);

  static void write(std::ostream& out, const Track& track);
};

class GPXError : public Exception {
public:
  GPXError(const std::string& msg)
      : Exception("Error parsing GPX: " + msg) {}
};

#endif
