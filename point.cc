#include "point.h"

#include <math.h>

using namespace std;

#define pi 3.14159265358979323846

double deg2rad(double deg) {
  return (deg * pi / 180);
}

double rad2deg(double rad) {
  return (rad * 180 / pi);
}

Point::Point()
    : lat(0), lon(0), elevation(0), length(0), timestamp(0),
      seq(0), hr(0), atemp(INVALID_TEMP), grade(0), velocity(0), climb(0)
{}

double Point::distance(const Point & other) const {

    double dlon = deg2rad(other.lon - lon);
    double dlat = deg2rad(other.lat - lat);

    double a = pow(sin(dlat / 2), 2) + cos(deg2rad(lat)) *
        cos(deg2rad(other.lat)) *
        pow(sin(dlon / 2), 2);
    return 2 * 6371000 * asin(sqrt(a));
}

bool Point::operator< (const Point & p) const {
    if (lat < p.lat) return true;
    if (lon < p.lon) return true;
    return (length < p.length);
}

ostream & operator << (ostream & out, const Point & p) {
    streamsize s = out.precision();
    out.precision(16);
    out << p.lat << " "
        << p.lon << " "
        << p.elevation << " "
        << p.timestamp << " "
        << p.seq << " "
        << p.hr << " "
        << p.atemp << " "
        << p.length << " "
        << p.grade << " "
        << p.velocity << " "
        << p.climb;
    out.precision(s);
    return out;
}
