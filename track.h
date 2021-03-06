#if !defined TRACK_H
#define      TRACK_H

#include "point.h"

#include <string>
#include <vector>

// A sequence of recorded points that defines a path.
class Track : public std::vector<Point> {
public:
  struct Peak {
    int index;         // index of Point
    double prominence; // minimum saddle height in meters
    double range;      // distance (meters) to higher ground
  };

  class Climb {
   public:
    Climb(const Track * trackIn, int startIn, int endIn)
        : track(trackIn), start(startIn), end(endIn) {}

    double getGrade() const;  // percent
    double getLength() const; // distance in meters
    double getClimb() const;  // elevation in meters
    double getDifficulty() const;

    const Point & getStart() const { return (*track)[start]; }
    const Point & getEnd() const { return (*track)[end]; }

    unsigned getStartIndex() const { return start; }
    unsigned getEndIndex() const { return end; }

    void SetStart(unsigned s) { start = s; }
    void SetEnd(unsigned e) { end = e; }

  private:
    const Track* track;
    unsigned start;
    unsigned end;
  };

  Track() {}

  void setName(const std::string & n) { name = n; }
  const std::string & getName() const { return name; }

  // Handy utilities, which fail if 'empty()'
  const Point& first() const;
  const Point& last() const;

  // Trim the track to 'samples' points
  void ShrinkBySample(unsigned samples);

  // Shrink the track by averaging points down to the desired size
  void ShrinkByAverage(unsigned points);

  // Remove points within the given rectangle
  void Mask(double minLon, double maxLon,
            double minLat, double maxLat);

  // Remove a few points at the beginning or end that result from failing
  // to reset the computer.
  void RemoveBurrs();

  // Set the length at each point by calculating the distance from the
  // previous point. This is normally done when reading the data, but
  // some formats (.fit) use a different method.
  void CalculateLength();

  // Adjust the elevation by applying a decaying average across the given
  // number of samples. More samples -> smoother elevation. This helps for
  // noisy elevation sources, notably GPS.
  void decayElevation(int samples);

  // Calculate a segment-oriented grade for each point, with segments of
  // the given length (in meters)
  void calculateSegmentGrade(double segmentLength);

  // Calculate the velocity of each point, using a decaying average with
  // the given number of samples
  void calculateVelocity(int samples);

  // Calculate the accumulated climbs. Annotate the points, and
  // return the total
  double calculateClimb(double threshold);

  // Time spent moving faster than the given limit (about 2 kph) 
  // in seconds
  double calculateMovingTime(double minVelocityMetersPerSec = 0.55556) const;

  // Return the total elapsed time of the ride, in seconds
  double calculateTotalTime() const;

  // Return average speed in meters/sec
  double calculateAverageSpeed() const;

  // Return moving speed in meters/sec
  double calculateMovingSpeed() const;

  // Return total distance in meters
  double getTotalDistance() const;

  // Return the (absolute) start time of the ride
  time_t getStartTime() const;

  // Return the end time of the ride
  time_t getEndTime() const;

  // Return an arbitrary number indicating the relative difficulty of
  // the track (positive grade squared times distance)
  double calculateDifficulty() const;

  // Find the distinct peaks in the track, with the given minimum
  // range (linear distance to taller point) and prominence (height
  // above saddle point), both in meters.
  void calculatePeaks(double minRange, double minProminence);

  // Identify distinct climbs, with the given inscrutable criteria.
  // minimumGrade: anything below this isn't a climb at all
  // gradeRatio: don't combine climbs if the resulting grade is less
  // than this, times the candidate grade. The idea is to avoid watering
  // down steep climbs with lesser beginnings or endings.
  //
  // A climb must satisfy at least *one* of the 'significant'
  // values -- length, grade or climb.
  //
  // minimumLength: anything shorter than this isn't a climb
  // twixtRatio: two climbs may be combined if the distance between is
  // less than this, times the length of the candidate
  void calculateClimbs(double minimumGrade,
                       double gradeRatio,
                       double significantLength,
                       double significantGrade,
                       double signficantClimb,
                       double minimumLength,
                       double twixtRatio);

  // Find the most 'difficult' section of 'meters' length, returning
  // the results in 'start' and 'end'. Difficult, in this context, is
  // measured with the same arbitrary algorithm as 'calculateDifficulty'
  void mostDifficult(int meters, int & start, int & end,
                     double & score) const;

  double getMaximumElevation() const;
  double getMinimumElevation() const;

  // Only non-empty if you've called calculatePeaks
  const std::vector<Peak>& getPeaks() const { return peaks; }

  // Only non-empty if you've called calculateClimbs
  const std::vector<Climb>& getClimbs() const { return climbs; }

 private:
  std::string name;
  std::vector<Peak> peaks;
  std::vector<Climb> climbs;
};

#endif
