/**********************************************************************

This is an attempt to identify similar routes. As it stands, it just
takes the points from one track, one by one, and finds the nearest
point in another track. Among other things, it doesn't concern itself
with ordering.

This is a pretty slow method, being O(n^2) with the number of points.
The good news is that you can abort the search with regard to a single
point if you find a close-enough point.  And you can abort the whole
comparison if you don't find a point very close at all.

***********************************************************************/

#include "track.h"
#include "util.h"
#include "parse.h"
#include "dir.h"

#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <set>

using namespace std;

static double trackDistance(const Track & left, const Track & right) {

    double leftDistance = 0;
    int comparisons = 0;

    // for each point in 'left', find the closest point in 'right'
    for (int i = 0; i < left.size(); i++) {

        double m = 1000000000;

        double latmin = left[i].lat - 0.001;
        double latmax = left[i].lat + 0.001;
        double lonmin = left[i].lon - 0.001;
        double lonmax = left[i].lon + 0.001;

        for (int j = 0; j < right.size(); j++) {

            if ((right[j].lat < latmin) ||
                (right[j].lat > latmax) ||
                (right[j].lon < lonmin) ||
                (right[j].lon > lonmax)) {
                continue;
            }

            comparisons++;
            double d = left[i].distance(right[j]);
            if (d < m) m = d;

            if (m < 5) break; // close enough
        }

        leftDistance += m;

        if (m > 1000) {
            leftDistance = m * left.size();
            break;
        }
    }

    return (leftDistance / left.size());
}

enum Result {
    RESULT_EQUAL,
    RESULT_CONTAINS,
    RESULT_ISCONTAINED,
    RESULT_NONE
};

static Result compare(const Track & left, const Track & right) {

    double leftAvg = trackDistance(left, right);
    double rightAvg = trackDistance(right, left);

    if ((leftAvg < 100) && (rightAvg < 100)) {
        return RESULT_EQUAL;
    } else if (leftAvg < 100) {
        return RESULT_ISCONTAINED;
    } else if (rightAvg < 100) {
        return RESULT_CONTAINS;
    } else {
        return RESULT_NONE;
    }
}

typedef set<string> Cluster;

int main(int argc, char * argv[]) {

    try {

        vector<Track *> tracks;

        for (int i = 1; i < argc; i++) {

            Track * t = new Track;
            Parse::read(*t, argv[i]);

            string n(t->getName());
            string s(Directory::basename(argv[i]));
            if (!n.empty()) {
                s += " (";
                s += n;
                s += ")";
            }
            t->setName(s);
            cerr << "Read " << t->getName() << endl;

            tracks.push_back(t);
        }

        vector<Cluster> clusters;

        for (int i = 0; i < tracks.size(); i++) {
            for (int j = i + 1; j < tracks.size(); j++) {
                Result res = compare(*tracks[i], *tracks[j]);

                switch (res) {
                case RESULT_EQUAL: cout << "equal: "; break;
                case RESULT_CONTAINS: cout << "contains: "; break;
                case RESULT_ISCONTAINED: cout << "is contained: "; break;
                case RESULT_NONE: cout << "none: "; break;
                }

                cout << tracks[i]->getName() << " "
                     << tracks[j]->getName() << endl;

                if (res == RESULT_EQUAL) {

                    Cluster * cluster = 0;
                    for (int k = 0; k < clusters.size(); k++) {
                        if (clusters[k].find(tracks[i]->getName()) !=
                            clusters[k].end()) {
                            cluster = &clusters[k];
                            break;
                        }
                    }

                    if (cluster == 0) {
                        Cluster newCluster;
                        newCluster.insert(tracks[i]->getName());
                        newCluster.insert(tracks[j]->getName());
                        clusters.push_back(newCluster);
                    } else {
                        cluster->insert(tracks[j]->getName());
                    }
                }
            }
        }

        cout << clusters.size() << " clusters." << endl;
        for (int i = 0; i < clusters.size(); i++) {
            cout << "Cluster " << i << ".";
            
            for (Cluster::const_iterator it = clusters[i].begin(); it != clusters[i].end(); it++) {
                cout << " " << *it;
            }
            cout << endl << endl;
        }

        return 0;

    } catch (const exception & e) {
        cerr << "Caught error in main: " << e.what() << endl;
    } catch (...) {
        cerr << "Caught unknown error in main" << endl;
    }

    return 1;
}
