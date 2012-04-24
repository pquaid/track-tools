#if !defined KML_H
#define      KML_H

#include <vector>
#include <string>
#include <exception>
#include <iosfwd>

#include "exception.h"
#include "track.h"


// Methods for reading and writing KML files
class KML {
public:
    static void getPoints(std::istream & in, Track & out);
    static void getPoints(const std::string & filename, Track & out);

    static void write(std::ostream & out, Track & track);
};

class KMLError : public Exception {
public:
   KMLError(const std::string & msg)
       : Exception("Error parsing KML: " + msg)
    {}
};

#endif
