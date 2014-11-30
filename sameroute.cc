/**********************************************************************

A work in progress.

This is an attempt to identify similar routes. As it stands, it just
takes the points from one track, one by one, and finds the nearest
point in another track. Among other things, it doesn't concern itself
with ordering.

This is a pretty slow method, being O(n^2) with the number of tracks,
then O(n^2) with the number of points. The good news is that you can abort
the comparison with regard to a single point if you find a close-enough point.
And you can abort the whole comparison if you don't find a point very close
at all.

***********************************************************************/

#include "dir.h"
#include "exception.h"
#include "parse.h"
#include "track.h"
#include "util.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <string>
#include <vector>

using namespace std;

static double trackDistance(const Track& left, const Track& right) {
  double leftDistance = 0;

  // for each point in 'left', find the closest point in 'right'
  for (const Point& left_point : left) {
    double m = 1000000000;

    double latmin = left_point.lat - 0.01;
    double latmax = left_point.lat + 0.01;
    double lonmin = left_point.lon - 0.01;
    double lonmax = left_point.lon + 0.01;

    for (const Point& right_point : right) {
      if ((right_point.lat < latmin) ||
          (right_point.lat > latmax) ||
          (right_point.lon < lonmin) ||
          (right_point.lon > lonmax)) {
        continue;
      }

      double d = left_point.distance(right_point);
      if (d < m) m = d;

      if (m < 2) break; // close enough
    }

    leftDistance += m * m;

    if (m > 1000000) {
      leftDistance = m * left.size();
      break;
    }
  }

  return (leftDistance / left.size());
}

struct Result {
  enum Judgement {
    RESULT_EQUAL,
    RESULT_CONTAINS,
    RESULT_ISCONTAINED,
    RESULT_NONE
  };

  Judgement judgement;
  double left_average;
  double right_average;
};


static Result compare(const Track& left, const Track& right) {
  Result result;
  result.left_average = trackDistance(left, right);
  result.right_average = trackDistance(right, left);

  const double close_enough = 500.0;
  if ((result.left_average < close_enough) &&
      (result.right_average < close_enough)) {
    result.judgement = Result::RESULT_EQUAL;
  } else if (result.left_average < close_enough) {
    result.judgement = Result::RESULT_ISCONTAINED;
  } else if (result.right_average < close_enough) {
    result.judgement = Result::RESULT_CONTAINS;
  } else {
    result.judgement = Result::RESULT_NONE;
  }

  return result;
}

int main(int argc, char* argv[]) {
  try {
    // A track, and the cluster to which the track belongs, if we know it.
    struct TrackInfo {
      Track track;
      set<Track*>* cluster = nullptr;
    };
    vector<TrackInfo> tracks;

    for (int i = 1; i < argc; ++i) {
      tracks.push_back(TrackInfo());
      Track* t = &tracks.rbegin()->track;

      Parse::read(argv[i], *t);

      string n(t->getName());
      string s(Directory::basename(argv[i]));
      if (!n.empty()) {
        s += " (";
        s += n;
        s += ")";
      }
      t->setName(s);
      t->RemoveBurrs();
      cerr << "Read " << t->getName() << endl;
    }

    // Each set is a cluster of equal tracks
    vector<set<Track*>*> clusters;

    for (TrackInfo& left : tracks) {
      if (left.cluster != nullptr) continue;

      for (TrackInfo& right : tracks) {
        if (&left == &right) continue;
        Result res = compare(left.track, right.track);

        switch (res.judgement) {
          case Result::RESULT_EQUAL: cerr << "equal: "; break;
          case Result::RESULT_CONTAINS: cerr << "contains: "; break;
          case Result::RESULT_ISCONTAINED: cerr << "is contained: "; break;
          case Result::RESULT_NONE: cerr << "none: "; break;
        }

        cerr << left.track.getName() << " " << res.left_average << " "
             << right.track.getName() << " " << res.right_average << endl;

        if (res.judgement == Result::RESULT_EQUAL) {
          if (left.cluster != nullptr) {
            left.cluster->insert(&right.track);
            if (right.cluster == nullptr) {
              right.cluster = left.cluster;
            }
          } else if (right.cluster != nullptr) {
            right.cluster->insert(&left.track);
            ASSERTION(left.cluster == nullptr);
            left.cluster = right.cluster;
          } else {
            clusters.push_back(new set<Track*>());
            set<Track*>* cluster = *clusters.rbegin();
            cluster->insert(&left.track);
            cluster->insert(&right.track);
            left.cluster = cluster;
            right.cluster = cluster;
          }
        }
      }
    }

    for (int i = 0; i < clusters.size(); ++i) {
      cerr << "Cluster " << i << ":" << endl;
      for (const auto* track : *clusters[i]) {
        cerr << "    " << track->getName() << endl;
      }
    }

    cout.precision(8);
    for (int i = 0; i < clusters.size(); ++i) {
      const set<Track*>& cluster = *clusters[i];
      cout << "set terminal pngcairo size 2000,2000" << endl;
      cout << "set output \"cluster_" << i << ".png\"" << endl;

      bool first_track = true;
      for (const Track* track : cluster) {
        if (first_track) {
          cout << "plot";
          first_track = false;
        } else {
          cout << ",";
        }
        cout << " '-' with lines title '" << track->getName() << "' ";
      }
      cout << endl;

      for (const Track* track : cluster) {
        for (const Point& point : *track) {
          cout << point.lon << " " << point.lat << endl;
        }
        cout << "e" << endl;
      }
    }

    return 0;

  } catch (const exception & e) {
    cerr << "Caught error in main: " << e.what() << endl;
  } catch (...) {
    cerr << "Caught unknown error in main" << endl;
  }

  return 1;
}
