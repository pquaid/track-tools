#include "document.h"
#include "gpx.h"
#include "track.h"

#include <string>
#include <vector>

#include "string.h"
#include "time.h"

using namespace rapidxml;
using namespace std;

static double toDouble(const char* val) {
  char* end = 0;
  const double d = strtod(val, &end);
  if (end == val || end == 0 || *end != '\0') {
    throw GPXError("Unable to convert numeric value: " + string(val));
  }
  return d;
}

// Set the timezone, but reset it on exit
class TimezoneReset {
public:
  TimezoneReset(const std::string& zone) {
    const char* oldTZ = getenv("TZ");
    if (oldTZ != 0) tz = oldTZ;
    setenv("TZ", zone.c_str(), 1);
  }

  ~TimezoneReset() {
    if (tz.empty()) {
      unsetenv("TZ");
    } else {
      setenv("TZ", tz.c_str(), 1);
    }
  }

private:
  std::string tz;
};

static void processDoc(const Document& doc, Track& points) {
  const xml_node<>* trk = doc.getTop().first_node()->first_node("trk");
  if (trk == nullptr) throw GPXError("No <trk> element");

  const xml_node<>* name = trk->first_node("name");
  if (name != nullptr) {
    points.setName(name->value());
  }

  // I need mktime() to work in UTC
  TimezoneReset tzreset("UTC");

  for (xml_node<>* trkseg = trk->first_node();
       trkseg != nullptr;
       trkseg = trkseg->next_sibling()) {    
    if (strcmp(trkseg->name(), "trkseg") == 0) {
      for (xml_node<>* trkpt = trkseg->first_node("trkpt");
           trkpt != nullptr;
           trkpt = trkpt->next_sibling("trkpt")) {
        Point current;

        current.lat = toDouble(trkpt->first_attribute("lat")->value());
        current.lon = toDouble(trkpt->first_attribute("lon")->value());

        const xml_node<>* ele = trkpt->first_node("ele");
        if (ele != nullptr) {
          current.elevation = toDouble(ele->value());
        }

        if (points.empty()) {
          current.length = 0;
        } else {
          current.length = current.distance(points[points.size()-1]);
        }

        const xml_node<>* ts = trkpt->first_node("time");
        if (ts != nullptr) {
          struct tm time;
          strptime(ts->value(), "%Y-%m-%dT%H:%M:%S.000Z", &time);
          current.timestamp = mktime(&time);
        }

        const xml_node<>* ext = trkpt->first_node("extensions");
        if (ext != nullptr) {
          ext = ext->first_node("gpxtpx:TrackPointExtension");
          if (ext != nullptr) {
            const xml_node<>* hr = ext->first_node("gpxtpx:hr");
            if (hr != nullptr) {
              current.hr = strtol(hr->value(), 0, 10);
            }
            const xml_node<>* atemp = ext->first_node("gpxtpx:atemp");
            if (atemp != nullptr) {
              current.atemp = toDouble(atemp->value());
            }
          }
        }

        current.seq = points.size();
        points.push_back(current);
      }
    }
  }
}

void GPX::read(const string& filename, Track& points) {
  Document doc;
  doc.read(filename);
  processDoc(doc, points);
}

void GPX::read(istream& in, Track& points) {
  Document doc;
  doc.read(in);
  processDoc(doc, points);
}

static string toString(const time_t& t) {
  struct tm time;
  gmtime_r(&t, &time);
  char buffer[100];
  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S.000Z", &time);
  return string(buffer);
}

void GPX::write(ostream& out, const Track& track) {
  const char* header =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<gpx version=\"1.1\" creator=\"Track Tools\"\n"
"  xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd http://www.garmin.com/xmlschemas/GpxExtensions/v3 http://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd http://www.garmin.com/xmlschemas/TrackPointExtension/v1 http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd\"\n"
"  xmlns=\"http://www.topografix.com/GPX/1/1\"\n"
"  xmlns:gpxtpx=\"http://www.garmin.com/xmlschemas/TrackPointExtension/v1\"\n"
"  xmlns:gpxx=\"http://www.garmin.com/xmlschemas/GpxExtensions/v3\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n";

  out << header;
  out << "<metadata>";
  if (!track.empty()) {
    out << "<time>" << toString(track.first().timestamp) << "</time>";
  }
  out << "</metadata>" << endl;

  out << "<trk>" << endl;
  if (!track.getName().empty()) {
    out << "<name>" << track.getName() << "</name>" << endl;
  }
  out << "<trkseg>" << endl;
  for (int i = 0; i < track.size(); i++) {
    out.precision(12);
    out << "<trkpt lon=\"" << track[i].lon << "\" lat=\""
        << track[i].lat << "\">" << endl;
    out.precision(7);
    out << "<ele>" << track[i].elevation << "</ele>" << endl;

    if (track[i].timestamp != 0) {
      out << "<time>" << toString(track[i].timestamp) << "</time>"
          << endl;
    }

    if ((track[i].hr > 0) || track[i].validTemp()) {
      out << "<extensions><gpxtpx:TrackPointExtension>" << endl;
      if (track[i].hr > 0) {
        out << "<gpxtpx:hr>" << track[i].hr << "</gpxtpx:hr>" << endl;
      }
      if (track[i].validTemp()) {
        out << "<gpxtpx:atemp>" << track[i].atemp << "</gpxtpx:atemp>"
            << endl;
      }
      out << "</gpxtpx:TrackPointExtension></extensions>" << endl;
    }
    out << "</trkpt>" << endl;
  }
  out << "</trkseg>" << endl;
  out << "</trk>" << endl;
  out << "</gpx>" << endl;
}
