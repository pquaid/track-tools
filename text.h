#if !defined TEXT_H
#define      TEXT_H

#include <iosfwd>

class Track;

// A simple format, mainly for testing.
//
//  * Lines starting with '#' are comments, and ignored
//  * Lines starting with @ are points, with space-separated values in order:
//    - latitude
//    - longitude
//    - elevation (meters)
//    - timestamp (time_t)
//    - sequence #
//    - heart rate
//    - temperature (C)
//    - cumulative distance (meters)
//    - instantaneous or segment grade (%)
//    - instantaneous velocity (meters/sec)
//    - cumulative climb (meters)
//  * Other lines set attributes, in the form 'key=value'. Supported keys:
//    - name (track name)

class Text {
public:
  static void read(std::istream& in, Track& track);
  static void write(std::ostream& out, const Track& track);
};

#endif
