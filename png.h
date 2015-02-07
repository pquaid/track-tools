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
    bool metric = false;    // true => metric, false => imperial
    bool grade = true;      // Indicate the grade?
    bool climbs = true;     // Indicate climbs?
    bool difficult = true;  // Show the most difficult km?
    bool days = false;      // Indicate days, in a multi-day tour
    double pointSize = 10;  // Font size for labels

    // The range of elevations to display, in terms of the displayed units
    // (feet or meters). Normally the range is calculated from the track, but
    // this lets you specify a wider range, so a flat track doesn't look
    // mountainous. If the actual elevations exceed this range, the
    // actual values will be used.
    double minimum_elevation = DBL_MAX;
    double maximum_elevation = DBL_MIN;

    std::string font = "helvetica";
  };

  static void write(std::ostream& out, const Track& track,
                    Options opt = Options());
};

#endif
