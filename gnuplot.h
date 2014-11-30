#if !defined GNUPLOT_H
#define      GNUPLOT_H

#include <iostream>
#include <string>

class Track;

// Methods for generating a gnuplot script for creating an
// elevation profile

class Gnuplot {
public:
  struct Options {
    Options() {}

    std::string terminal = "pngcairo size 800,300";
    bool metric = false;
    bool grade = true;
    bool elevation = true;
    bool climbs = true;
    bool difficult = true;
  };

  // Create a gnuplot script, based on the track
  static void write(std::ostream& out, const Track& track,
                    Options opt = Options());
};

#endif
