#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "document.h"
#include "track.h"
#include "kml.h"

using namespace std;
using namespace rapidxml;

// Parse a single "longitude latitude elevation" triplet
static void parseCoord(const string& coord, const time_t ts, Track& track) {
  istringstream c(coord);
  Point p;

  c >> p.lon >> p.lat >> p.elevation;

  p.seq = track.size();

  p.length = 0;
  if (!track.empty()) {
    p.length = track.last().length + p.distance(track.last());
  }

  p.timestamp = ts;
  track.push_back(p);
}

// Inefficient parsing of a series of long,lat,elevation triplets
static void parseCoords(const string& coordString,
                        const time_t timestamp,
                        Track& track) {
  istringstream coords(coordString);

  while (coords.good() && !coords.eof()) {
    string coord;
    coords >> coord;
    if (coord.empty()) break;

    for (int i = 0; i < coord.size(); i++) {
      if (coord[i] == ',') coord[i] = ' ';
    }

    parseCoord(coord, timestamp, track);
  }
}

// Surely there's a strptime format suited for this, including the
// fractional seconds? I didn't see one, hence the following.
static time_t parseTimestamp(const char* timestamp) {
  struct tm time;
  time.tm_wday = 0;
  time.tm_yday = 0;
  time.tm_isdst = 0;

  float seconds = 0;

  const int result = sscanf(timestamp, "%d-%d-%dT%d:%d:%fZ",
                            &time.tm_year, &time.tm_mon, &time.tm_mday,
                            &time.tm_hour, &time.tm_min, &seconds);
  if (result != 6) {
    throw KMLError("Invalid timestamp: '" + string(timestamp) + "'");
  }

  time.tm_year -= 1900;
  --time.tm_mon;
  --time.tm_hour;
  time.tm_sec = roundf(seconds);

  return mktime(&time);
}

static void processDoc(Document& doc, Track& track) {
  xml_node<>* kml = doc.getTop().first_node("kml");
  if (kml == nullptr) {
    throw KMLError("No <kml> element");
  }

  const char* containerName = "Folder";
  const xml_node<>* container = kml->first_node(containerName);
  if (container == nullptr) {
    containerName = "Document";
    container = kml->first_node(containerName);
  }

  if (container != nullptr) {
    const xml_node<>* name = container->first_node("name");
    if (name != nullptr) {
      track.setName(name->value());
    }
  }

  for ( ;
        container != nullptr;
        container = container->next_sibling(containerName)) {
    for (const auto* placemark = container->first_node("Placemark");
         placemark != nullptr;
         placemark = placemark->next_sibling("Placemark")) {
      const xml_node<>* linestring = placemark->first_node("LineString");

      if (linestring != nullptr) {
        const xml_node<>* coords = linestring->first_node("coordinates");
        if (coords != nullptr) {
          parseCoords(coords->value(), 0, track);
        }
      } else {
        // There was no LineString. Maybe there's a gx:MultiTrack?
        const xml_node<>* multitrack = placemark->first_node("gx:MultiTrack");

        if (multitrack != nullptr) {
          for (const auto* gx_track = multitrack->first_node("gx:Track");
               gx_track != nullptr;
               gx_track = gx_track->next_sibling("gx:Track")) {
            time_t last_timestamp = 0;
            for (const auto* node = gx_track->first_node();
                 node != nullptr;
                 node = node->next_sibling()) {
              if (string("when") == node->name()) {
                last_timestamp = parseTimestamp(node->value());
              } else if (string("gx:coord") == node->name()) {
                parseCoord(node->value(), last_timestamp, track);
              }
            }
          }
        }
      }
    }
  }
}

void KML::read(const string& filename, Track& track) {
  Document doc;
  doc.read(filename);
  processDoc(doc, track);
}

void KML::read(istream& in, Track& track) {
  Document doc;
  doc.read(in);
  processDoc(doc, track);
}

static void writePoint(ostream& out, const Point& point) {
  out.precision(12);
  out << point.lon << "," << point.lat << ",";
  out.precision(6);
  out << point.elevation << endl;
}

static void writeSegment(ostream& out, const Track& track,
			 int start, int end, const string& style) {
  out << "<Placemark>" << endl
      << "<styleUrl>" << style << "</styleUrl>" << endl
      << "<LineString>" << endl
      << "<extrude>false</extrude>" << endl
      << "<tessellate>true</tessellate>" << endl
      << "<altitudeMode>clampToGround</altitudeMode>" << endl
      << "<coordinates>" << endl;

  for (int i = start; i <= end; ++i) {
    writePoint(out, track[i]);
  }

  out << "</coordinates></LineString></Placemark>" << endl;
}

void KML::write(ostream& out, const Track& track, Options options) {
  streamsize precision = out.precision();

  string name(track.getName());
  if (name.empty()) name = "Path";

  out << R"(<?xml version="1.0" encoding="UTF-8"?>
<kml xmlns="http://earth.google.com/kml/2.2">
<Document>
<name>)" << name << R"(</name>
<Style id="FlatStyle">
<LineStyle><color>)";

  char buffer[100];
  snprintf(buffer, sizeof(buffer), "%02x%06x", options.opacity, options.color);

  out << buffer << "</color><width>" << options.width
      << "</width></LineStyle></Style>" << endl;
  out << R"(<Style id="ClimbStyle">
<LineStyle><color>ff0000bb</color><width>2</width></LineStyle></Style>
)";

  /*
  // TODO: To activate this, I need to ensure that Track::sample() properly
  // adjusts climbs and peaks, which refer to points by index.
  std::vector<Track::Climb> climbs = track.getClimbs();
  int last_point = 0;
  for (const Track::Climb& climb : climbs) {
    if (climb.getStartIndex() > last_point) {
      writeSegment(out, track, last_point, climb.getStartIndex(),
		   "#FlatStyle");
    }
    writeSegment(out, track, climb.getStartIndex(), climb.getEndIndex(),
		 "#ClimbStyle");
    last_point = climb.getEndIndex();
  }
  if (last_point <= (track.size() - 1)) {
    writeSegment(out, track, last_point, track.size() - 1, "#FlatStyle");
  }
  */
  writeSegment(out, track, 0, track.size() - 1, "#FlatStyle");

  if (options.startAndEnd && !track.empty()) {
    out << R"(<Placemark><name>Start</name>
<Style><IconStyle><scale>1.3</scale><Icon>
<href>http://maps.google.com/mapfiles/kml/paddle/grn-circle.png</href>
</Icon>
<hotSpot yunits="fraction" y="0.0" xunits="fraction" x="0.5"/>
</IconStyle></Style>
<Point><coordinates>)";
    out.precision(12);
    out << track.first().lon << "," << track.first().lat << ",";
    out.precision(6);
    out << track.first().elevation
	<< "</coordinates></Point></Placemark>" << endl;
        
    out << R"(<Placemark><name>End</name>
<Style><IconStyle><scale>1.3</scale><Icon>
<href>http://maps.google.com/mapfiles/kml/paddle/red-circle.png</href>
</Icon>
<hotSpot yunits="fraction" y="0.0" xunits="fraction" x="0.5"/>
</IconStyle></Style>
<Point><coordinates>)";
    out.precision(12);
    out << track.last().lon << "," << track.last().lat << ",";
    out.precision(6);
    out << track.last().elevation
	<< "</coordinates></Point></Placemark>" << endl;
  }

  out << "</Document>" << endl << "</kml>" << endl;

  out.precision(precision);
}
