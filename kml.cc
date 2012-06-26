#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "document.h"
#include "track.h"
#include "kml.h"

using namespace std;
using namespace rapidxml;

// Inefficient parsing of KML coordinate triplet
static void parseCoords(const string & coordString,
                        Track & track) {

    istringstream coords(coordString);

    while (coords.good() && !coords.eof()) {

        string coord;
        coords >> coord;
        if (coord.empty()) break;

        for (int i = 0; i < coord.size(); i++) {
            if (coord[i] == ',') coord[i] = ' ';
        }

        istringstream c(coord);

        Point p;

        c >> p.lon >> p.lat >> p.elevation;

        p.seq = track.size();

        p.length = 0;
        if (!track.empty()) {
            p.length = track.last().length + p.distance(track.last());
        }

        track.push_back(p);
    }
}

static void processDoc(Document & doc, Track & track) {

    xml_node<> * kml = doc.getTop().first_node("kml");
    if (kml == 0) {
        throw KMLError("No <kml> element");
    }

    const char * containerName = "Folder";
    xml_node<> * container = kml->first_node(containerName);
    if (container == 0) {
        containerName = "Document";
        container = kml->first_node(containerName);
    }

    if (container != 0) {
        xml_node<> * name = container->first_node("name");
        track.setName(name->value());
    }

    for ( ; container != 0; container = container->next_sibling(containerName)) {
        xml_node<> * placemark = container->first_node("Placemark");
        if (placemark != 0) {

            xml_node<> * linestring = placemark->first_node("LineString");
            if (linestring != 0) {
                xml_node<> * coords = linestring->first_node("coordinates");
                if (coords != 0) {
                    parseCoords(coords->value(), track);
                }
            }
        }
    }
}

void KML::read(const string & filename, Track & track) {

    Document doc;
    doc.read(filename);
    processDoc(doc, track);
}

void KML::read(istream & in, Track & track) {

    Document doc;
    doc.read(in);
    processDoc(doc, track);
}



void KML::write(ostream & out, Track & track, Options options) {

    streamsize precision = out.precision();

    string name(track.getName());
    if (name.empty()) name = "Path";

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl
        << "<kml xmlns=\"http://earth.google.com/kml/2.2\">" << endl
        << "<Document>" << endl
        << "<name>" << name << "</name>" << endl
        << "<Placemark>" << endl
        << "<name>" << name << "</name>" << endl
        << "<Style><LineStyle>" << endl
        << "<color>";

    char buffer[100];
    snprintf(buffer, sizeof(buffer), "%02x%06x",
             options.opacity, options.color);

    out << buffer << "</color><width>" << options.width << "</width>" << endl
        << "</LineStyle></Style>" << endl
        << "<LineString>" << endl
        << "<extrude>false</extrude>" << endl
        << "<tessellate>true</tessellate>" << endl
        << "<altitudeMode>clampToGround</altitudeMode>" << endl
        << "<coordinates>" << endl;

    for (int i = 0; i < track.size(); i++) {
        out.precision(12);
        out << track[i].lon << "," << track[i].lat << ",";
        out.precision(6);
        out << track[i].elevation << endl;
    }


    out << "</coordinates>"
        << "</LineString></Placemark>" << endl;

    if (options.startAndEnd && !track.empty()) {

        out << "<Placemark><name>Start</name>" << endl
            << "<Style><IconStyle><scale>1.3</scale><Icon>" << endl
            << "<href>http://maps.google.com/mapfiles/kml/paddle/grn-circle.png</href>"
            << "</Icon>" << endl
            << "<hotSpot yunits=\"fraction\" y=\"0.0\" xunits=\"fraction\" x=\"0.5\"/>" << endl
            << "</IconStyle></Style>" << endl
            << "<Point><coordinates>";
        out.precision(12);
        out << track.first().lon << "," << track.first().lat << ",";
        out.precision(6);
        out << track.first().elevation
            << "</coordinates></Point></Placemark>" << endl;
        
        out << "<Placemark><name>End</name>" << endl
            << "<Style><IconStyle><scale>1.3</scale><Icon>" << endl
            << "<href>http://maps.google.com/mapfiles/kml/paddle/red-circle.png</href>"
            << "</Icon>" << endl
            << "<hotSpot yunits=\"fraction\" y=\"0.0\" xunits=\"fraction\" x=\"0.5\"/>" << endl
            << "</IconStyle></Style>" << endl
            << "<Point><coordinates>";
        out.precision(12);
        out << track.last().lon << "," << track.last().lat << ",";
        out.precision(6);
        out << track.last().elevation
            << "</coordinates></Point></Placemark>" << endl;
    }

    out << "</Document>" << endl
        << "</kml>" << endl;

    out.precision(precision);
}
