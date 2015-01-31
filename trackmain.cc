#include "dir.h"
#include "document.h"
#include "fit.h"
#include "gnuplot.h"
#include "gpx.h"
#include "json.h"
#include "kml.h"
#include "png.h"
#include "parse.h"
#include "text.h"
#include "track.h"
#include "util.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

void usage() {
  cerr << "Usage: track [-options] [input-files]" << endl
       << "Options:" << endl
       << "             -a <int> (alpha for KML line, 0..255)" << endl
       << "             -b <lon-1>,<lon-2>,<lat-1>,<lat-2> (mask)" << endl
       << "             -c (calculate climbs)" << endl
       << "             -d (calculate most difficult KM)" << endl
       << "             -e (omit start/end in KML)" << endl
       << "             -f <input-file> " << endl
       << "             -h <int> (elevation decay samples)" << endl
       << "             -i <input-format> (gpx, kml, fit, txt -- optional)"
       << endl
       << "             -j <name> (JSON callback function)" << endl
       << "             -k min,max (displayed elevation range)" << endl
       << "             -l (KML line color - BGR, eg 0xBBGGRR)" << endl
       << "             -m (metric)" << endl
       << "             -n <int> (average down to 'n' samples)" << endl
       << "             -o <output-format> (gpx, kml, gnuplot, txt)" << endl
       << "             -p (calculate peaks)" << endl
       << "             -q (quiet; print nothing extra)" << endl
       << "             -r (calculate length relative to previous points)"
       << endl
       << "             -s <int> (sample)" << endl
       << "             -t <string> (gnuplot terminal)" << endl
       << "             -u width,height (PNG image dimensions)" << endl
       << "             -v (verbose; default)" << endl
       << "             -w <double> (width of KML line)" << endl
       << endl
       << "The -i parameter is optional if the filename ends with" << endl
       << "one of: .gpx, .kml, .fit or .txt" << endl;
}

// Command-line options
static string input_filename;
static Parse::Format input_format  = Parse::FORMAT_UNKNOWN;
static Parse::Format output_format = Parse::FORMAT_UNKNOWN;

static bool doClimbs     = true;
static bool doPeaks      = false;
static bool doDifficult  = true;
static int  downsample   = 0;
static bool quiet        = false;
static bool metric       = false;
static int  average      = 0;
static string jsonCallback;
static bool remove_burrs  = true;
static bool relativeLength = false;
static int decaySamples  = 0;

static bool doMask       = false;
static double maskMinLat = 0;
static double maskMinLon = 0;
static double maskMaxLat = 0;
static double maskMaxLon = 0;

static double min_elevation = DBL_MAX;
static double max_elevation = DBL_MIN;

static int image_width = 0;
static int image_height = 0;
static string gnuplot_terminal = "";

static KML::Options kmlOptions;

static double altitude(double meters) {
  if (metric) {
    return meters;
  } else {
    return meters * 3.2808;
  }
}

static double distance(double meters) {
  if (metric) {
    return meters / 1000.0;
  } else {
    return (meters / 1000.0) * 0.62137;
  }
}

static void report(Track& track) {
  double climb = 0;
  if (!track.empty()) {
    climb = track.last().climb;
  }

  cerr << "Track:         " << track.getName() << endl;
  cerr << "Climb:         " << (int) altitude(climb)
       << (metric ? " meters" : " feet") << endl;
  cerr << "Effort:        " << ((int) track.calculateDifficulty())
       << endl;
  cerr << "Moving time:   " << Util::asTime(track.calculateMovingTime())
       << endl;
  cerr << "Total time:    " << Util::asTime(track.calculateTotalTime())
       << endl;
  cerr << "Distance:      " << distance(track.getTotalDistance())
       << (metric ? " km" : " miles")
       << endl;

  const double total_hours = track.calculateTotalTime() / 3600.0;
  cerr << "Average speed: " << distance(track.getTotalDistance()) / total_hours
       << (metric ? " kph" : " mph") << endl;
  const double moving_hours = track.calculateMovingTime() / 3600.0;
  cerr << "Moving speed:  " << distance(track.getTotalDistance()) / moving_hours
       << (metric ? " kph" : " mph") << endl;
  cerr << "Data points: " << track.size() << endl;
}

static void calculatePeaks(Track& track) {
  track.calculatePeaks(2000, 50);

  if (!quiet) {
    for (const Track::Peak& peak : track.getPeaks()) {
      cerr << "^ at " << distance(track[peak.index].length)
           << ", ele " << (int) altitude(track[peak.index].elevation)
           << ", prom " << (int) altitude(peak.prominence)
           << ", range " << distance(peak.range) << endl;
    }
  }
}

static void calculateClimbs(Track& track) {
  track.calculateClimbs(4,     // minimum grade, in percent
                        0.8,   // ratio of grades for joining climbs
                        1000,  // significant length in meters
                        6,     // significant grade, in percent
                        100,   // significant climb, in meters
                        600,   // minimum length, in meters
                        0.35); // maximum distance between climbs, as a ratio

  if (quiet) return;

  const vector<Track::Climb>& climbs(track.getClimbs());
  for (int i = 0; i < climbs.size(); ++i) {
    const Track::Climb& climb = climbs[i];
    cerr << "Climb " << i + 1 << ". From "
         << distance(climb.getStart().length) << " to "
         << distance(climb.getEnd().length) << " ("
         << distance(climb.getLength())
         << (metric ? " km" : " miles")
         << ") grade " << climb.getGrade() << ". From "
         << altitude(climb.getStart().elevation) << " to "
         << altitude(climb.getEnd().elevation)
         << " ("
         << altitude(climb.getEnd().elevation - climb.getStart().elevation)
         << (metric ? " meters)" : " feet)")
         << " - " << (int) climb.getDifficulty() << endl;
  }
}

static void calculateDifficult(const Track& track) {
  if (track.empty()) return;
  
  int start = 0;
  int end = 0;
  double score = 0;
  track.mostDifficult(1000, start, end, score);

  if (quiet) return;

  if (start < 0) {
    cerr << "Easy ride" << endl;
  } else {
    double dist = track[end].length - track[start].length;
    double ele = track[end].elevation - track[start].elevation;

    cerr << "Most difficult KM: "
         << distance(track[start].length)
         << " to " << distance(track[end].length)
         << " " << 100.0*(ele / dist) << "%, "
         << altitude(ele) << (metric ? " m" : " ft")
         << " of climb (" << score << ")"
         << endl;
  }
}

static void processMask(string arg) {
  for (int i = 0; i < arg.size(); i++) {
    if (arg[i] == ',') arg[i] = ' ';
  }

  istringstream in(arg);
  in >> maskMinLon >> maskMaxLon >> maskMinLat >> maskMaxLat;

  if ((maskMinLon < -180) || (maskMinLon > 180)) {
    throw Exception("mask min longitude must be between -180 and 180");
  }

  if ((maskMaxLon < -180) || (maskMaxLon > 180) || (maskMaxLon < maskMinLon)) {
    throw Exception("mask max longitude must be between -180 and 180, "
                    "and greater than min");
  }

  if ((maskMinLat < -90) || (maskMinLat > 90)) {
    throw Exception("mask min latitude must be between -90 and 90");
  }
  if ((maskMaxLat < -90) || (maskMaxLat > 90) || (maskMaxLat < maskMinLat)) {
    throw Exception("mask min latitude must be between -90 and 90, "
                    "and greater than min");
  }
}

static void processCommandLine(int argc, char * argv[]) {
  while (true) {
    const int opt = getopt(argc, argv, "a:b:cdef:h:i:j:k:l:mn:o:pqrs:t:u:vw:y");
    if (opt == -1) break;

    switch (opt) {
      case 'a':
        kmlOptions.opacity = strtol(optarg, 0, 0);
        if (kmlOptions.opacity > 255) {
          throw Exception("-a (opacity) must be between 0 and 255");
        }
        break;

      case 'b':
        doMask = true;
        processMask(optarg);
        break;

      case 'c':
        doClimbs = false;
        break;
  
      case 'd':
        doDifficult = false;
        break;

      case 'e':
        kmlOptions.startAndEnd = false;
        break;

      case 'f':
        input_filename = optarg;
        break;

      case 'h':
        decaySamples = strtol(optarg, 0, 0);
        if (decaySamples <= 0) {
          throw Exception("-h (decay samples) must be positive");
        }
        break;

      case 'i':
        input_format = Parse::stringToFormat(optarg);
        if (input_format == Parse::FORMAT_PNG) {
          throw Exception("Unable to read PNG files");
        } else if (input_format == Parse::FORMAT_JSON) {
          throw Exception("Unable to read JSON files");
        } else {
          if (input_format == Parse::FORMAT_UNKNOWN) {
            throw Exception(string("Unknown input format '") +
                            optarg + "'");
          }
        }
        break;    

      case 'j':
        jsonCallback = optarg;
        break;

      case 'k':
        if (sscanf(optarg, "%lf,%lf", &min_elevation, &max_elevation) != 2) {
          throw Exception("Expecting -k min,max. Example: -k 100,1000");
        }
        if (min_elevation > max_elevation) {
          throw Exception("Minimum elevation must be > maximum");
        }
        break;

      case 'l':
        kmlOptions.color = strtol(optarg, 0, 0);
        if (kmlOptions.color > 0xffffff) {
          throw Exception("-l (line color) must be 0 to 0xffffff");
        }
        break;

      case 'm':
        metric = true;
        break;

      case 'n':
        average = strtol(optarg, 0, 0);
        if (average < 0) {
          throw Exception("-n (average) must be greater than 0");
        }
        break;

      case 'o':
        output_format = Parse::stringToFormat(optarg);
        if (output_format == Parse::FORMAT_FIT) {
          throw Exception("Unable to write FIT files");
        } else if (output_format == Parse::FORMAT_UNKNOWN) {
          throw Exception(string("Unknown output format '") +
                          optarg + "'");
        }
        break;

      case 'p':
        doPeaks = true;
        break;

      case 'q':
        quiet = true;
        break;

      case 'r':
        relativeLength = true;
        break;

      case 's':
        downsample = strtol(optarg, 0, 0);
        if (downsample < 1) {
          throw Exception("-s (downsample) must be greater than 0");
        }
        break;

      case 't':
        gnuplot_terminal = optarg;
        break;

      case 'u':
        if (sscanf(optarg, "%d,%d", &image_width, &image_height) != 2) {
          throw Exception("Expecting -t width,height. Example: -t 800,300");
        }
        if (image_width <= 0 || image_height <= 0) {
          throw Exception("Invalid image dimensions (must be > 0)");
        }
        break;

      case 'v':
        quiet = false;
        break;

      case 'w':
        kmlOptions.width = strtod(optarg, 0);
        break;

      case 'y':
        remove_burrs = !remove_burrs;
        break;

      default:
        throw Exception("Unknown option");
    }
  }

  for ( ; optind < argc; optind++) {      
    if (input_filename.empty()) {
      input_filename = argv[optind];
    } else {
      throw Exception(string("Extra parameter detected: ") +
                      argv[optind]);
    }
  }
}

static string removeSuffix(const string& str) {
  string::size_type pos = str.find_last_of('.');
  if ((pos != string::npos) && (pos != 0)) {
    return str.substr(0,pos);
  } else {
    return str;
  }
}

int main(int argc, char * argv[]) {
  // Figure out the input, do some calculations, write the
  // output (if any)

  try {
    processCommandLine(argc, argv);

    // Read it
    Track track;
    Parse::read(input_filename, track, input_format);

    if (track.getName().empty()) {
      track.setName(removeSuffix(Directory::basename(input_filename)));
    }

    if (remove_burrs) {
      track.RemoveBurrs();
    }

    if (relativeLength) {
      track.CalculateLength();
    }

    if (decaySamples > 0) {
      track.decayElevation(decaySamples);
    }

    if (average > 0) {
      track.ShrinkByAverage(average);
    }

    // Do some calculations
    track.calculateSegmentGrade(100);
    double climb = track.calculateClimb(10);
    track.calculateVelocity(10);

    if (!quiet) report(track);

    if (doMask) {
      track.Mask(maskMinLon, maskMaxLon, maskMinLat, maskMaxLat);
    }

    if (doPeaks)     calculatePeaks(track);
    if (doClimbs)    calculateClimbs(track);
    if (doDifficult) calculateDifficult(track);
    if (downsample > 0)  track.ShrinkBySample(downsample);

    // Write the results, if desired
    if (output_format == Parse::FORMAT_GNUPLOT) {
      Gnuplot::Options opt;
      opt.metric = metric;
      if (!gnuplot_terminal.empty()) {
        opt.terminal = gnuplot_terminal;
      }
      Gnuplot::write(cout, track, opt);
    } else if (output_format == Parse::FORMAT_KML) {
      KML::write(cout, track, kmlOptions);
    } else if (output_format == Parse::FORMAT_GPX) {
      GPX::write(cout, track);
    } else if (output_format == Parse::FORMAT_TEXT) {
      Text::write(cout, track);
    } else if (output_format == Parse::FORMAT_PNG) {
      PNG::Options opt;
      opt.metric = metric;
      opt.climbs = doClimbs;
      opt.difficult = doDifficult;
      opt.minimum_elevation = min_elevation;
      opt.maximum_elevation = max_elevation;
      if (image_width > 0) opt.width = image_width;
      if (image_height > 0) opt.height = image_height;
      PNG::write(cout, track, opt);
    } else if (output_format == Parse::FORMAT_JSON) {
      JSON::Options opt;
      opt.callback = jsonCallback;
      JSON::write(cout, track, opt);
    } else {
      cerr << "Nothing to write" << endl;
    }

    return 0;

  } catch (const std::exception& e) {
    cerr << e.what() << endl;
  } catch (...) {
    cerr << "Unknown exception in main" << endl;
  }

  usage();
  return 1;
}
