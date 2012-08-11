#include "track.h"
#include "gpx.h"
#include "fit.h"
#include "kml.h"
#include "document.h"
#include "util.h"
#include "gnuplot.h"
#include "gpx.h"
#include "png.h"
#include "json.h"
#include "text.h"
#include "parse.h"
#include "dir.h"

#include <sys/time.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

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
         << "             -i <input-format> (gpx, kml, fit, txt -- optional)" << endl
         << "             -j <name> (JSON callback function)" << endl
         << "             -l (KML line color - BGR, eg 0xBBGGRR)" << endl
         << "             -m (metric)" << endl
         << "             -n <int> (average down to 'n' samples)" << endl
         << "             -o <output-format> (gpx, kml, gnuplot, txt)" << endl
         << "             -p (calculate peaks)" << endl
         << "             -q (quiet; print nothing extra)" << endl
         << "             -s <int> (sample)" << endl
         << "             -v (verbose; default)" << endl
         << "             -w <double> (width of KML line)" << endl
         << endl
         << "The -i parameter is optional if the filename ends with" << endl
         << "one of: .gpx, .kml, .fit or .txt" << endl;
}

// Command-line options
static string inputFilename;
static Parse::Format inputFormat  = Parse::FORMAT_UNKNOWN;
static Parse::Format outputFormat = Parse::FORMAT_UNKNOWN;
static bool plotOutput = false;

static bool doClimbs     = true;
static bool doPeaks      = false;
static bool doDifficult  = true;
static int  downsample   = 0;
static bool quiet        = false;
static bool metric       = false;
static int  average      = 0;
static string jsonCallback;

static bool doMask       = false;
static double maskMinLat = 0;
static double maskMinLon = 0;
static double maskMaxLat = 0;
static double maskMaxLon = 0;

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

static void report(Track & track) {

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

    double speed = track.getTotalDistance() / track.calculateTotalTime();
    cerr << "Average speed: "
         << (metric ? (speed*3.6) : (speed*2.2369))
         << (metric ? " kph" : " mph") << endl;
    speed = track.getTotalDistance() / track.calculateMovingTime();
    cerr << "Moving speed:  "
         << (metric ? (speed*3.6) : (speed*2.2369))
         << (metric ? " kph" : " mph") << endl;
    cerr << "Data points: " << track.size() << endl;
}

static void calculatePeaks(Track & track) {

    track.calculatePeaks(2000, 50);

    if (!quiet) {

        const vector<Track::Peak> & peaks(track.getPeaks());
        for (int i = 0; i < peaks.size(); i++) {
            cerr << "^ at " << distance(track[peaks[i].index].length)
                 << " miles, ele "
                 << (int) altitude(track[peaks[i].index].elevation)
                 << " ft, prom "
                 << (int) altitude(peaks[i].prominence) << " ft, range "
                 << distance(peaks[i].range) << " miles" << endl;
        }
    }
}

static void calculateClimbs(Track & track) {

    track.calculateClimbs(4, 0.8, 1000, 6, 100, 600, 0.35);

    if (quiet) return;

    const vector<Track::Climb> & climbs(track.getClimbs());
    for (int i = 0; i < climbs.size(); i++) {
        cerr << "Climb " << i + 1 << ". From "
             << distance(climbs[i].getStart().length) << " to "
             << distance(climbs[i].getEnd().length) << " ("
             << distance(climbs[i].getLength())
             << (metric ? " km" : " miles")
             << ") grade " << climbs[i].getGrade() << ". From "
             << altitude(climbs[i].getStart().elevation) << " to "
             << altitude(climbs[i].getEnd().elevation)
             << " (" << altitude(climbs[i].getEnd().elevation - climbs[i].getStart().elevation)
             << (metric ? " meters)" : " feet)")
             << " - " << (int) climbs[i].getDifficulty() << endl;
    }
}

static void calculateDifficult(Track & track) {

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
        throw Exception("mask max longitude must be between -180 and 180, greater than min");
    }

    if ((maskMinLat < -90) || (maskMinLat > 90)) {
        throw Exception("mask min latitude must be between -90 and 90");
    }
    if ((maskMaxLat < -90) || (maskMaxLat > 90) || (maskMaxLat < maskMinLat)) {
        throw Exception("mask min latitude must be between -90 and 90, and greater than min");
    }
}

static void processCommandLine(int argc, char * argv[]) {

    while (true) {
        int opt = getopt(argc, argv, "a:b:cdef:i:j:l:mn:o:pqs:vw:");
        if (opt == -1) break;

        switch (opt) {
        case 'a':
            kmlOptions.opacity = strtol(optarg, 0, 0);
            if (kmlOptions.opacity > 255) {
                throw Exception("-a (opacity) must be between 0 and 255");
            }
            break;
        case 'e':
            kmlOptions.startAndEnd = false;
            break;
        case 'f':
            inputFilename = optarg;
            break;

        case 'j':
            jsonCallback = optarg;
            break;

        case 'l':
            kmlOptions.color = strtol(optarg, 0, 0);
            if (kmlOptions.color > 0xffffff) {
                throw Exception("-l (line color) must be 0 to 0xffffff");
            }
            break;
        case 'o':
            if (optarg == string("gnuplot")) {
                outputFormat = Parse::FORMAT_UNKNOWN;
                plotOutput = true;
            } else {
                outputFormat = Parse::stringToFormat(optarg);
                if (outputFormat == Parse::FORMAT_FIT) {
                    throw Exception("Unable to write FIT files");
                } else if (outputFormat == Parse::FORMAT_UNKNOWN) {
                    throw Exception(string("Unknown output format '") +
                                    optarg + "'");
                }
            }
            break;
        case 'i':
            inputFormat = Parse::stringToFormat(optarg);
            if (inputFormat == Parse::FORMAT_PNG) {
                throw Exception("Unable to read PNG files");
            } else if (inputFormat == Parse::FORMAT_JSON) {
                throw Exception("Unable to read JSON files");
            } else {
                if (inputFormat == Parse::FORMAT_UNKNOWN) {
                    throw Exception(string("Unknown input format '") +
                                    optarg + "'");
                }
            }
            break;
            
        case 'c':
            doClimbs = false;
            break;

        case 'p':
            doPeaks = true;
            break;

        case 'd':
            doDifficult = false;
            break;

        case 's':
            downsample = strtol(optarg, 0, 0);
            if (downsample < 1) {
                throw Exception("-s (downsample) must be greater than 0");
            }
            break;

        case 'q':
            quiet = true;
            break;

        case 'v':
            quiet = false;
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

        case 'b':
            doMask = true;
            processMask(optarg);
            break;

        case 'w':
            kmlOptions.width = strtod(optarg, 0);
            break;

        default:
            throw Exception("Unknown option");
        }
    }

    for ( ; optind < argc; optind++) {
        
        if (inputFilename.empty()) {
            inputFilename = argv[optind];
        } else {
            throw Exception(string("Extra parameter detected: ") +
                            argv[optind]);
        }
    }
}

static string removeSuffix(const string & str) {

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
        Parse::read(track, inputFilename, inputFormat);

        if (track.getName().empty()) {
            track.setName(removeSuffix(Directory::basename(inputFilename)));
        }

        if (average > 0) {
            track.average(average);
        }

        // Do some calculations
        track.calculateSegmentGrade(100);
        double climb = track.calculateClimb(10);
        track.calculateVelocity(10);

        if (!quiet) report(track);

        if (doMask) {
            track.mask(maskMinLon, maskMaxLon,
                       maskMinLat, maskMaxLat);
        }

        if (doPeaks)     calculatePeaks(track);
        if (doClimbs)    calculateClimbs(track);
        if (doDifficult) calculateDifficult(track);
        if (downsample > 0)  track.sample(downsample);

        // Write the results, if desired
        if (plotOutput) {
            Gnuplot::Options opt;
            opt.metric = metric;
            Gnuplot::write(cout, track, opt);
        } else if (outputFormat == Parse::FORMAT_KML) {
            KML::write(cout, track, kmlOptions);
        } else if (outputFormat == Parse::FORMAT_GPX) {
            GPX::write(cout, track);
        } else if (outputFormat == Parse::FORMAT_TEXT) {
            Text::write(cout, track);
        } else if (outputFormat == Parse::FORMAT_PNG) {
            PNG::Options opt;
            opt.metric = metric;
            opt.climbs = doClimbs;
            opt.difficult = doDifficult;
            PNG::write(cout, track, opt);
        } else if (outputFormat == Parse::FORMAT_JSON) {
            JSON::Options opt;
            opt.callback = jsonCallback;
            JSON::write(cout, track, opt);
        } else {
            cerr << "Nothing to write" << endl;
        }

        return 0;

    } catch (const std::exception & e) {
        cerr << e.what() << endl;
    } catch (...) {
        cerr << "Unknown exception in main" << endl;
    }

    usage();
    return 1;
}
