#if !defined PNG_H
#define      PNG_H

#include <cfloat>
#include <iosfwd>
#include <string>

class Track;

class PNG {
public:
  struct Options {
    Options() {}

    int width = 800;
    int height = 300;
    bool metric = false;
    bool grade = true;
    bool climbs = true;
    bool difficult = true;
    double pointSize = 10;

    double minimum_elevation = DBL_MAX;
    double maximum_elevation = DBL_MIN;

    std::string font = "helvetica";
  };

  static void write(std::ostream& out, const Track& track,
                    Options opt = Options());
};

#endif
