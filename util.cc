
#include "util.h"

#include <string>
#include <string.h>
#include <stdio.h>

using namespace std;

const double kMilesPerKilometer = 0.62137;
const double kFeetPerMeter = 3.2808;

bool Util::endsWith(const string& str, const string& suffix) {
  return (suffix.size() <= str.size() &&
          (strncmp(suffix.data(),
                   str.data() + (str.size() - suffix.size()),
                   suffix.size()) == 0));
}

double Util::meters_to_miles(const double meters) {
  return (meters / 1000.0) * kMilesPerKilometer;
}

double Util::meters_to_feet(const double meters) {
  return meters * kFeetPerMeter;
}

string Util::asTime(const time_t secIn) {
  int seconds = secIn;
  char buffer[1000];

  if (seconds < 60) {
    snprintf(buffer, sizeof(buffer), "%d", seconds);
  } else if (seconds < 3600) {
    snprintf(buffer, sizeof(buffer), "%02d:%02d", seconds / 60, seconds % 60);
  } else if (seconds < 86400) {
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
             seconds / 3600, (seconds % 3600) / 60, seconds % 60);
  } else {
    const int days = seconds / 86400;
    snprintf(buffer, sizeof(buffer), "%d day%s, %02d:%02d:%02d",
             days, (days > 1 ? "s" : "") , (seconds % 86400) / 24,
             (seconds % 3600) / 60, seconds % 60);
  }

  return string(buffer);
}
