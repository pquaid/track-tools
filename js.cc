#include "js.h"
#include <iostream>
#include "track.h"

using namespace std;

void JS::write(ostream & out, Track & track) {

    out << "var trackData = new Object();" << endl;
    out << "trackData.points = [";
    for (int i = 0; i < track.size(); i++) {
        if (i > 0) out << "," << endl;
        out << "["
            << track[i].length << "," << track[i].elevation
            << "]";
    }
    out << "];" << endl;
    out << "trackData.climbs = [" << endl;
    for (int i = 0; i < track.getClimbs().size(); i++) {
        const Track::Climb & climb(track.getClimbs()[i]);
        if (i > 0) out << "," << endl;
        out << "{ start: " << climb.getStart().seq
            << ", end: " << climb.getEnd().seq << " }";
    }
    out << "];" << endl;
    {
        int start = 0;
        int end = 0;
        double score = 0;
        track.mostDifficult(1000, start, end, score);
        out << "trackData.difficult = { start: " << start
            << ", end: " << end << ", score: " << score << "};"
            << endl;
    }
}
