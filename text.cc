#include "text.h"

#include <iostream>
#include <sstream>

#include <ctype.h>

#include "exception.h"
#include "point.h"
#include "track.h"

using namespace std;

static string trim(string s) {
  while (!s.empty() && isspace(s[0])) s.erase(0,1);
  while (!s.empty() && isspace(s[s.size()-1])) s.resize(s.size()-1);
  return s;
}

void Text::read(istream& in, Track& track) {
  while (in.good()) {
    string line;
    getline(in, line);

    if (line.empty()) continue;
    if (line[0] == '#') continue;

    if (line[0] != '@') {
      string::size_type s = line.find('=');
      if ((s == string::npos) || (s == 0) || (s == (line.size()-1))) {
        throw Exception("Bad line in text file: '" + line + "'");
      }

      string key = trim(line.substr(0,s));
      string value = trim(line.substr(s+1));

      if (key == "name") {
        track.setName(value);
      } else {
        throw Exception("Unknown attribute: '" + key + "'");
      }
      continue;
    }

    istringstream lineStream(line);
    string token;

    Point current;
    lineStream >> token
               >> current.lat
               >> current.lon
               >> current.elevation
               >> current.timestamp
               >> current.seq
               >> current.hr
               >> current.atemp
               >> current.length
               >> current.grade
               >> current.velocity
               >> current.climb;

    // This ought to be right, even if the file's screwy
    current.seq = track.size();

    track.push_back(current);
  }
}

void Text::write(ostream& out, const Track& track) {
  if (!track.getName().empty()) {
    out << "name=" << track.getName() << endl;
  }

  out << "# lat lon ele timestamp seq hr atemp length grade velocity climb"
      << endl;

  streamsize s = out.precision();

  for (const Point& point : track) {
    out.precision(16);
    out << "@ " << point.lat << " " << point.lon;
    out.precision(6);
    out << " " << point.elevation
        << " " << point.timestamp
        << " " << point.seq
        << " " << point.hr
        << " " << point.atemp;
    out.precision(8);
    out << " " << point.length;
    out.precision(4);
    out << " " << point.grade
        << " " << point.velocity
        << " " << point.climb
        << endl;
  }
  out.precision(s);
}
