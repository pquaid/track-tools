#include "json.h"
#include <iostream>
#include "track.h"

using namespace std;

void JSON::write(ostream & out, Track & track, JSON::Options opt) {

    if (!opt.callback.empty()) {
        out << opt.callback << "(";
    }
    out << "{ \"points\": [";
    for (int i = 0; i < track.size(); i++) {
        if (i > 0) out << "," << endl;
        out << "["
            << track[i].length << "," << track[i].elevation
            << "]";
    }
    out << "]," << endl;
    out << "\"climbs\": [" << endl;
    for (int i = 0; i < track.getClimbs().size(); i++) {
        const Track::Climb & climb(track.getClimbs()[i]);
        if (i > 0) out << "," << endl;
        out << "{ \"start\": " << climb.getStart().seq
            << ", \"end\": " << climb.getEnd().seq << " }";
    }
    out << "]," << endl;
    {
        int start = 0;
        int end = 0;
        double score = 0;
        track.mostDifficult(1000, start, end, score);
        out << "\"difficult\": { \"start\": " << start
            << ", \"end\": " << end << ", \"score\": " << score << "}"
            << endl;
    }
    out << "}" << endl;
    if (!opt.callback.empty()) {
        out << ");" << endl;
    }
}
