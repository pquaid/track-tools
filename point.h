#if !defined POINT_H
#define      POINT_H

#include <iostream>
#include <time.h>
#include <string>

// A recorded point in a track; basically a time & lat/lon, with a
// few other attached metrics.
struct Point {
  constexpr double INVALID_TEMP = -1000;

  Point();

  // Return distance in meters between 'this' and 'other'
  double distance(const Point & other) const;

  bool validTemp() const { return atemp != INVALID_TEMP; }

  // Order two points by lat, lon, and ele
  bool operator< (const Point & other) const;

  double lat;            // latitude in degrees
  double lon;            // longitude in degrees
  double elevation;      // elevation in meters
  double length;         // distance within track
  time_t timestamp;      // timestamp of data point
  int seq;               // sequential # of point, in track
  int hr;                // heart rate, in beats/minute
  double atemp;          // air temperature, in C
  double grade;          // change in elevation, in %
  double velocity;       // velocity, in m/s
  double climb;          // accumulated climb, in meters
};

std::ostream & operator << (std::ostream & out, const Point & p);

#endif
