
#include "parse.h"
#include "util.h"
#include "exception.h"
#include "gpx.h"
#include "fit.h"
#include "kml.h"
#include "text.h"
#include "track.h"

#include <fstream>
#include <iostream>

using namespace std;


void Parse::read(Track & track, const std::string & filename, Format format) {

    if ((format == FORMAT_UNKNOWN) && !filename.empty()) {

        if (Util::endsWith(filename, ".gpx")) {
            format = FORMAT_GPX;
        } else if (Util::endsWith(filename, ".fit")) {
            format = FORMAT_FIT;
        } else if (Util::endsWith(filename, ".kml")) {
            format = FORMAT_KML;
        } else if (Util::endsWith(filename, ".txt")) {
            format = FORMAT_TEXT;
        }
    }

    if (format == FORMAT_UNKNOWN) {
        throw Exception("Could not deduce input file format");
    }

    ifstream infile;
    istream * in;
    if (filename.empty() || (filename == "-")) {
        in = &cin;
    } else {
        infile.open(filename.c_str(), ios_base::in|ios_base::binary);
        if (!infile.is_open()) {
            throw Exception("Could not open " + filename);
        }
        in = &infile;
    }
    POSTCONDITION(in != 0);

    if (format == FORMAT_GPX) {
        GPX::read(*in, track);
    } else if (format == FORMAT_FIT) {
        Fit::read(*in, track);
    } else if (format == FORMAT_KML) {
        KML::read(*in, track);
    } else if (format == FORMAT_TEXT) {
        Text::read(*in, track);
    } else {
        ASSERTION(false);
    }
}


Parse::Format Parse::stringToFormat(const std::string & format) {

    if (format == "gpx") {
        return FORMAT_GPX;
    } else if (format == "kml") {
        return FORMAT_KML;
    } else if (format == "fit") {
        return FORMAT_FIT;
    } else if (format == "txt") {
        return FORMAT_TEXT;
    } else {
        return FORMAT_UNKNOWN;
    }
}
