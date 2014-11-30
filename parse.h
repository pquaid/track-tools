#if !defined PARSE_H
#define      PARSE_H

#include <string>

class Track;

class Parse {
public:
  // Note that some of these are output-only.
  enum Format {
    FORMAT_GPX,
    FORMAT_KML,
    FORMAT_FIT,
    FORMAT_TEXT,
    FORMAT_PNG,
    FORMAT_JSON,
    FORMAT_GNUPLOT,
    FORMAT_UNKNOWN
  };

  static void read(const std::string& filename, Track& track,
                   Format format = FORMAT_UNKNOWN);

  static Format stringToFormat(const std::string& format);
};

#endif
