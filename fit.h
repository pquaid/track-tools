#if !defined FIT_H
#define      FIT_H

#include <vector>
#include <iosfwd>

#include "exception.h"

class Track;

class Fit {
public:
  // Read a FIT-formatted activity file into the track
  static void read(std::istream& in, Track& points);
};

class FitException : public Exception {
public:
  FitException(const std::string& msg)
      : Exception("Error decoding FIT file: " + msg)
  {}
};

#endif
