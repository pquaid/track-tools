#include "track.h"
#include "exception.h"

#include <sstream>
#include <memory>
#include <stdint.h>

using namespace std;

double Track::Climb::getGrade() const {
    return (getClimb() / getLength()) * 100.0;
}

double Track::Climb::getLength() const {
    return (*track)[end].length - (*track)[start].length;
}

double Track::Climb::getClimb() const {
    return (*track)[end].elevation - (*track)[start].elevation;
}

double Track::Climb::getDifficulty() const {
    return getGrade() * getGrade() * getLength();
}



const Point & Track::first() const {
    PRECONDITION(!empty());
    return (*this)[0];
}

const Point & Track::last() const {
    PRECONDITION(!empty());
    return (*this)[size()-1];
}

// Remove some of the entries to get down to 'samples'. At the moment
// this is just approximate; due to integer math, it gets close to
// 'samples', but not exact
void Track::sample(int samples) {

    if (samples >= size()) return;

    /* This is slower, as written, and doesn't produce a visibly better
       sample.

    // Sample based on distance
    double each = getTotalDistance() / (samples-1);
    double pos = 0;

    int i = 0;
    while (i < size()-1) {

        if (at(i).length < pos) {
            erase(begin() + i);
        } else {
            pos += each;
            i++;
        }
    }
    */

    // Sample every n'th point
    int each = size() / samples;
    if (each < 1) each = 1;

    int i = 0;
    while (i < size()) {

        i++; // skip one entry;
        int end = i + each - 1;
        if (end >= size()) end = size() - 1;
        if (end > i) {
            erase(begin() + i, begin() + end);
        }
    }
}

void Track::average(int points) {

    PRECONDITION(points > 0);

    if (points >= size()) return;

    vector<Point> replacements;

    for (int i = 0; i < points; i++) {

        int start = (((double) i) / points) * size();
        int end   = (((double) i + 1) / points) * size();

        POSTCONDITION(end <= size());

        Point p;
        p = at(start);
        p.seq = i;

        int64_t t = p.timestamp;

        for (int j = start + 1; j < end; j++) {

            p.lat       += at(j).lat;
            p.lon       += at(j).lon;
            p.elevation += at(j).elevation;
            p.length    += at(j).length;
            p.hr        += at(j).hr;
            p.atemp     += at(j).atemp;
            p.grade     += at(j).grade;
            p.velocity  += at(j).velocity;
            p.climb     += at(j).climb;

            t += at(j).timestamp;
        }

        int count = end - start;

        if (count > 1) {
            p.lat       /= count;
            p.lon       /= count;
            p.elevation /= count;
            p.length    /= count;
            p.hr        /= count;
            p.atemp     /= count;
            p.grade     /= count;
            p.velocity  /= count;
            p.climb     /= count;

            p.timestamp = t / count;
        }

        p.grade = 0;

        replacements.push_back(p);
    }

    clear();
    assign(replacements.begin(), replacements.end());
}

// Remove points within the given rectangle
void Track::mask(double minLon, double maxLon,
                 double minLat, double maxLat) {

    int i = 0;
    while (i < size()) {

        Point & p = (*this)[i];

        if ((p.lat <= minLat) ||
            (p.lat >= maxLat) ||
            (p.lon <= minLon) ||
            (p.lon >= maxLon)) {

            i++;

        } else {
            erase(begin() + i);
        }
    }
}

// Decay the elevation. This helps if you're using GPS elevation, which
// is very noisy
void Track::decayElevation(int samples) {

    double running;
    for (int i = 0; i < size(); i++) {

        if (i == 0) {
            running = (*this)[i].elevation;
        } else {
            running = ((*this)[i].elevation + running * (samples-1)) / samples;
            (*this)[i].elevation = running;
        }
    }
}

static bool matchesPattern(Track & track, int pos) {

    // The pattern is: three points increasing or decreasing.

    if (pos < 2) return false;

    if ((track[pos-2].elevation < track[pos-1].elevation) &&
        (track[pos-1].elevation < track[pos].elevation)) {
        return true;
    }

    if ((track[pos-2].elevation > track[pos-1].elevation) &&
        (track[pos-1].elevation > track[pos].elevation)) {
        return true;
    }

    return false;
}

// Calculate the average grade over segments of the given length. Since
// GPS can be noisy, calculating the grade over longer segments (100
// meters, for example) gives more realistic results.
void Track::calculateSegmentGrade(double segmentLength) {

    int segmentStartIndex = 0;
    double segmentStartElevation = 0;
    double segmentStartDistance = 0;

    double windowStart = segmentLength * 0.9;
    double windowEnd   = segmentLength * 1.1;

    for (int i = 0; i < size(); i++) {

        if (i == 0) {

            segmentStartElevation = at(i).elevation;
            segmentStartDistance  = 0;

        } else {

            double deltaD = at(i).length - segmentStartDistance;
            if ((deltaD >= windowEnd) ||
                ((deltaD >= windowStart) && matchesPattern(*this, i))) {

                // Calculate the grade
                double deltaE = at(i).elevation - segmentStartElevation;
                double grade = (deltaE / deltaD) * 100;

                // Fill in the points in this active segment
                for (int j = segmentStartIndex; j < i; j++) {
                    at(j).grade = grade;
                }

                segmentStartIndex     = i;
                segmentStartElevation = at(i).elevation;
                segmentStartDistance  = at(i).length;
            }
        }
    }

    // Fill in the last segment
    if (segmentStartIndex < size()) {
        ASSERTION(!empty());
        double deltaE = last().elevation - segmentStartElevation;
        double deltaD = last().length - segmentStartDistance;
        double grade = (deltaE / deltaD) * 100;
        if (deltaD == 0) grade = 0;

        for (int i = segmentStartIndex; i < size(); i++) {
            at(i).grade = grade;
        }
    }
}

// Set the velocity of each point. Since GPS is noisy, use a decaying
// average.

void Track::calculateVelocity(int samples) {

    if (empty()) return;

    time_t previous = at(0).timestamp;
    double running = 0;

    at(0).velocity = 0;

    for (int i = 1; i < size(); i++) {

        double distance = at(i).length - at(i-1).length;
        double diff = at(i).timestamp - previous;

        if (diff > 0) {
            double mps = distance / diff;
            running = (mps + running * (samples-1)) / samples;
        }

        at(i).velocity = running;
        previous = at(i).timestamp;
    }
}

// Calculate the total climb, but ignore small variations below
// a threshold.

double Track::calculateClimb(double threshold) {

    double base = 0;
    double climb = 0;

    for (int i = 0; i < size(); i++) {

        double ele = at(i).elevation;
        if (i == 0) {
            base = ele;
        } else {

            if (ele > (base + threshold)) {
                climb += ele - base;
                base = ele;
            }
            if (ele < base) {
                base = ele;
            }
        }

        at(i).climb = climb;
    }

    return climb;
}


double Track::calculateMovingTime(double minVelocityMetersPerSec) const {

    double moving = 0;
    double stopped = 0;

    for (int i = 1; i < size(); i++) {

        time_t now  = at(i).timestamp;
        time_t prev = at(i-1).timestamp;
        double dist = at(i).length - at(i-1).length;

        if (prev < now) {
            double vel = dist / (now - prev);
            if (vel > minVelocityMetersPerSec) {
                moving += (now - prev);
            } else {
                stopped += (now - prev);
            }
        }
    }

    return moving;
}


double Track::calculateTotalTime() const {

    if (empty()) {
        return 0;
    } else {
        return last().timestamp - first().timestamp;
    }
}


// Return average speed in meters/sec
double Track::calculateAverageSpeed() const {
    return getTotalDistance() / calculateTotalTime();
}


// Return moving speed in meters/sec
double Track::calculateMovingSpeed() const {
    return getTotalDistance() / calculateMovingTime();
}


// Return total distance in meters
double Track::getTotalDistance() const {
    if (empty()) {
        return 0;
    } else {
        return last().length;
    }
}


time_t Track::getStartTime() const {
    if (empty()) {
        return 0;
    } else {
        return first().timestamp;
    }
}

time_t Track::getEndTime() const {
    if (empty()) {
        return 0;
    } else {
        return last().timestamp;
    }
}

double Track::getMaximumElevation() const {

    PRECONDITION(!empty());

    double max = at(0).elevation;
    for (int i = 1; i < size(); i++) {
        if (at(i).elevation > max) max = at(i).elevation;
    }

    return max;
}

double Track::getMinimumElevation() const {

    PRECONDITION(!empty());

    double min = at(0).elevation;
    for (int i = 1; i < size(); i++) {
        if (at(i).elevation < min) min = at(i).elevation;
    }

    return min;
}


// Return an arbitrary number indicating the relative difficulty of
// the track
double Track::calculateDifficulty() const {

    double result;
    for (int i = 1; i < size(); i++) {
        double grade = at(i).grade;
        if (grade >= 0) {
            result += grade * grade * (at(i).length - at(i-1).length);
        }
    }

    return result;
}


void Track::calculatePeaks(double range, double prom) {

    peaks.clear();

    // Identify any point with a prominence of at least 'prom' meters
    // in a range of at least 'range' meters.

    int sz = size();

    // Since we're scanning this vector a jillion times, creating a
    // raw copy is noticably faster.

    typedef Point* PointPtr;
    auto_ptr<PointPtr> pointHolder(new PointPtr[sz]);
    Point ** pts = pointHolder.get();
    for (int i = 0; i < sz; i++) {
        pts[i] = &(*this)[i];
    }

    for (int i = 0; i < sz; i++) {

        // Calculate the prominence and range of every single point
        double promPre = -1;
        double promPost = -1;
        double rangePre = -1;
        double rangePost = -1;

        for (int j = i - 1; j >= 0; j--) {
            double delta = pts[i]->elevation - pts[j]->elevation;
            if (delta <= 0) {
                rangePre = pts[i]->length - pts[j]->length;
                break;
            }
            if (delta > promPre) {
                promPre = delta;
            }
        }

        if (rangePre < 0) rangePre = pts[i]->length;

        for (int j = i + 1; j < sz; j++) {
            double delta = pts[i]->elevation - pts[j]->elevation;
            if (delta < 0) {
                rangePost = pts[j]->length - pts[i]->length;
                break;
            }
            if (delta > promPost) {
                promPost = delta;
            }
        }

        if (rangePost < 0) rangePost = pts[sz-1]->length - pts[i]->length;

        if ((promPre >= prom) &&
            (promPost >= prom) &&
            (rangePre >= range) &&
            (rangePost >= range)) {

            Peak p;
            p.index = i;
            p.prominence = std::min(promPre, promPost);
            p.range      = std::min(rangePre, rangePost);
            
            peaks.push_back(p);
        }
    }
}

typedef vector<pair<Point,Point> > ClimbWork;

static double getGrade(const Point & start, const Point & end) {
    double ele = end.elevation - start.elevation;
    double len = end.length - start.length;
    return (ele / len) * 100.0;
}

static void getBaseClimbs(const Track & points,
                          ClimbWork & climbs,
                          double minimumGrade) {

    for (int i = 0; i < points.size(); i++) {
        
        if (points[i].grade >= minimumGrade) {

            int end;

            // Collect all contiguous points with a minimal grade
            for (end = i + 1; end < points.size(); end++) {
                if (points[end].grade < minimumGrade) break;
            }

            if (end == points.size()) end--;

            climbs.push_back(make_pair(points[i], points[end]));
            i = end;
        }
    }
}

static void combineWithNext(ClimbWork & climbs,
                            double twixtRatio,
                            double gradeRatio,
                            double minimumGrade) {

    int i = 0;
    while (!climbs.empty() && (i < (climbs.size()-1))) {

        double length = climbs[i].second.length - climbs[i].first.length;
        double toNext = climbs[i+1].first.length - climbs[i].second.length;

        double startGrade = getGrade(climbs[i].first, climbs[i].second);
        double totalGrade = getGrade(climbs[i].first, climbs[i+1].second);

        // If the climbs are pretty close, relative to their length,
        //    and the distance between isn't too far (regardless of length),
        //    and the resulting grade is pretty close to this grade,
        //    and the resulting grade is above the minimum,
        //    and the following climb finishes higher than this one,
        // then combine

        if ((toNext < (length * twixtRatio)) &&
            (toNext < 500) &&
            (totalGrade >= (startGrade * gradeRatio)) &&
            (totalGrade >= minimumGrade) &&
            (climbs[i].second.elevation < climbs[i+1].second.elevation)) {

            // combine 'em
            climbs[i+1].first = climbs[i].first;
            climbs.erase(climbs.begin() + i);

        } else {
            i++;
        }
    }
}

static void combineWithPrevious(ClimbWork & climbs,
                                double twixtRatio,
                                double gradeRatio,
                                double minimumGrade) {

    int i = climbs.size()-1;
    while (i > 0) {

        double length = climbs[i].second.length - climbs[i].first.length;
        double toNext = climbs[i].first.length - climbs[i-1].second.length;
        
        double startGrade = getGrade(climbs[i].first, climbs[i].second);
        double totalGrade = getGrade(climbs[i-1].first, climbs[i].second);

        if ((toNext < (length * twixtRatio)) &&
            (toNext < 500) &&
            (totalGrade >= (startGrade * gradeRatio)) &&
            (totalGrade >= minimumGrade) &&
            (climbs[i-1].first.elevation < climbs[i].first.elevation)) {

            // combine 'em
            climbs[i-1].second = climbs[i].second;
            climbs.erase(climbs.begin() + i);
        }
        i--;
    }
}

static void removeInsignificant(ClimbWork & climbs,
                                double significantLength,
                                double significantGrade,
                                double significantClimb,
                                double minimumLength,
                                double minimumGrade) {

    // Remove climbs that are not "significant". It has to be steep,
    // long or have a lot of climb. And it has to have (at least) a
    // minimal length and grade.

    int i = 0;
    while (i < climbs.size()) {

        double steep = getGrade(climbs[i].first, climbs[i].second);
        double length = climbs[i].second.length - climbs[i].first.length;
        double ele = climbs[i].second.elevation - climbs[i].first.elevation;

        // Any of these criteria...
        bool significant =
            (length > significantLength) ||
            (steep > significantGrade) ||
            (ele > significantClimb);

        // And all of the minimums...
        significant = significant &&
            (length >= minimumLength) &&
            (steep >= minimumGrade);

        if (significant) {
            i++;
        } else {
            climbs.erase(climbs.begin() + i);
        }
    }
}

void Track::calculateClimbs(double minimumGrade,
                            double gradeRatio,
                            double significantLength,
                            double significantGrade,
                            double significantClimb,
                            double minimumLength,
                            double twixtRatio) {

    climbs.clear();

    // We will collect climbs in a local structure first
    vector<pair<Point,Point> > local;

    getBaseClimbs(*this, local, minimumGrade);

    // Combine nearby sections

    while (true) {

        int lengthPre = local.size();
        
        combineWithNext(local, twixtRatio, gradeRatio, minimumGrade);
        combineWithPrevious(local, twixtRatio, gradeRatio, minimumGrade);
            
        if (local.size() == lengthPre) break;
    }

    removeInsignificant(local,
                        significantLength,
                        significantGrade,
                        significantClimb,
                        minimumLength,
                        minimumGrade);

    for (int i = 0; i < local.size(); i++) {
        climbs.push_back(Climb(this, local[i].first.seq, local[i].second.seq));
    }
}

void Track::mostDifficult(int meters, int & start, int & end, double & score) {

    // Find the most difficult stretch of 'meters'.

    // Here's what I'll do: for each point, create a 'pain' number
    // (grade^^2 * length), then just scan once for the most difficult
    // stretch.
    //
    // Squaring the grade matches reality, I think, but also unfortunately
    // exaggerates sensor errors. So to combat that, I'm using an extended
    // average. I'm also *not* using the segment grade, because that is also
    // too susceptible to sensor error.

    if (empty()) return;

    auto_ptr<double> pain(new double[size()]);

    const int SAMPLES = 10;
    double runningGrade = 0;
    pain.get()[0] = 0;

    for (int i = 1; i < size(); i++) {

        double ele = at(i).elevation - at(i-1).elevation;
        double length = at(i).length - at(i-1).length;
        double grade = 100 * (ele / length);
        if (length <= 1) {
            grade = runningGrade;
        }

        // We need to avoid crazy spikes in grade due to sensor error
        runningGrade = (runningGrade * (SAMPLES-1) + grade) / SAMPLES;

        if (runningGrade > 0) {
            pain.get()[i] = (runningGrade * runningGrade) * length;
        } else {
            pain.get()[i] = 0;
        }
    }

    // Now just find the most difficult 'meters' span

    score = 0;
    start = -1;
    end = -1;

    int s = 0; // candidate start
    double total = 0;

    for (int i = 1; i < size(); i++) {

        total += pain.get()[i];
        while ((at(i).length - at(s).length) > meters) {
            total -= pain.get()[s];
            s++;
        }

        // We use the fact that we've bumped 's' as an indication that
        // distance(s,i) is approximately 'meters' long. So, yeah, the
        // first point will never be included.
        
        if (s > 0) {
            if (total > score) {
                score = total;
                start = s;
                end = i;
            }
        }
    }
}
