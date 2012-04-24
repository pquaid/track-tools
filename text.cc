#include "text.h"

#include <iostream>
#include <sstream>

#include "track.h"
#include "exception.h"

using namespace std;

void Text::getPoints(istream & in, Track & track) {

    while (in.good()) {

        string line;
        getline(in, line);

        if (line.empty()) continue;
        if (line[0] == '#') continue;

        if (line[0] != '@') {

            string::size_type s = line.find('=');
            if (s == string::npos) {
                throw Exception("Bad line in text file: '" + line + "'");
            }

            // parse the attribute, assign it
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

void Text::write(ostream & out, Track & track) {

    if (!track.getName().empty()) {
        out << "name=" << track.getName() << endl;
    }

    out << "# lat lon elevation timestamp seq hr atemp length grade velocity climb" << endl;

    streamsize s = out.precision();
    out.precision(12);

    for (int i = 0; i < track.size(); i++) {

        out.precision(16);
        out << "@ " << track[i].lat
            << " " << track[i].lon;
        out.precision(6);
        out << " " << track[i].elevation
            << " " << track[i].timestamp
            << " " << track[i].seq
            << " " << track[i].hr
            << " " << track[i].atemp;
        out.precision(8);
        out << " " << track[i].length;
        out.precision(4);
        out << " " << track[i].grade
            << " " << track[i].velocity
            << " " << track[i].climb
            << endl;
    }
    out.precision(s);
}
