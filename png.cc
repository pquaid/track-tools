#include "png.h"
#include "track.h"
#include "exception.h"

#include <iostream>
#include <stdio.h>
#include <math.h>

#include "gd.h"
#include "gdfontl.h"

using namespace std;

const double PI = 3.14159265359;
const double MILES_PER_KM = 0.62137;
const double FEET_PER_METER = 3.2808;

class Image {
public:
  Image(PNG::Options inOpts) : ptr(0), options(inOpts) {
    top = left = bottom = right = 0;
    min = max = incr = subIncr = length = 0;

    ptr = gdImageCreateTrueColor(options.width, options.height);
    POSTCONDITION(ptr != 0);

    gdFTUseFontConfig(1);
  }

  ~Image() { if (!ptr) gdImageDestroy(ptr); }

  // Return a non-const string to accommodate GD's general non-constness.
  char* font() const { return const_cast<char*>(options.font.c_str()); }

  // Return either km or miles, based on the option.
  double distance(double meters) const {
    if (options.metric) {
      return meters / 1000.0;
    } else {
      return (meters / 1000.0) * MILES_PER_KM;
    }
  }

  // Return either meters or feet, depending on the option.
  double elevation(double d) const {
    if (options.metric) {
      return d;
    } else {
      return d * FEET_PER_METER;
    }
  }

  // borders of the graph portion of the image, inside the labels
  int top, left, bottom, right;

  gdImagePtr ptr;

  PNG::Options options;

  // Max and min Y values, in terms of the displayed data (not the source
  // data in meters, not the pixels). So if it's metric, it's in meters,
  // otherwise in feet. The min and max are slightly below and above the
  // track's min and max elevation.
  double min;
  double max;
  double incr;       // tic increment
  double subIncr;    // sub-tic increment

  // Total length in meters.
  double length;

  // Return the X position (in pixels) of a given data value
  int getX(double pos_in_meters) const {
    return (pos_in_meters / length) * (right - left) + left;
  }

  // Return the Y position for a value already converted to display form
  // (ie metric or imperial). For example, a value based on 'min' or 'max'.
  int getRawY(double ele) const {
    return bottom - (ele - min) / (max - min) * (bottom - top);
  }

  // Return the Y position (in pixels) of a given data value
  int getY(double ele_in_meters) const {
    return getRawY(elevation(ele_in_meters));
  }
};

namespace {

// Writes a buffer of len bytes to context, which is an ostream.
int writeTo(void* context, const char* buffer, int len) {
  PRECONDITION(context != 0);
  PRECONDITION(buffer != 0);

  ostream* out = (ostream*) context;
  out->write(buffer, len);
  return len;
}

// Returns an integer between 'easy' and 'hard', based on the value of
// 'actual', which ranges from 0 to 1.0.
int intensity(int easy, int hard, double actual) {
  return (int) (easy * (1 - actual) + hard * actual);
}

void graphGrade(const Image& img, const Track& track, bool grade) {
  if (track.empty()) return;

  int prevX = 0;
  int prevY = 0;

  // Calculate the first point
  prevX = img.left;
  prevY = img.getY(track[0].elevation);

  gdPoint points[4];

  int color = gdImageColorResolve(img.ptr, 0x50, 0xb0, 0x50);
  int easy[3] = { 0x50, 0xb0, 0x50 };
  int hard[3] = { 0xff, 0x00, 0x00 };

  for (int i = 0; i < track.size(); i++) {
    int x = img.getX(track[i].length);
    int y = img.getY(track[i].elevation);

    points[0].x = prevX;
    points[0].y = prevY;

    points[1].x = x;
    points[1].y = y;

    points[2].x = x;
    points[2].y = img.bottom;

    points[3].x = prevX;
    points[3].y = img.bottom;

    if (grade) {
      // We're going to pick a color between "easy" and "hard". We'll
      // arbitrarily set "easy" to < 1% grade, and "hard" to 12% or more.
      // Grade is non-linear, so I'm going to square it (again,
      // arbitrarily).  But since this makes lower-grade climbs
      // indistinguishable from 'easy' in color, we're going to shift the
      // numbers up in the color space.

      const double MAXGRADE = 12;
      double h = track[i].grade;

      if (h < 1) h = 0;
      if (h > MAXGRADE) h = MAXGRADE;
            
      h = ((h+6) * (h+6)) / ((MAXGRADE+6)*(MAXGRADE+6));

      POSTCONDITION(h >= 0);
      POSTCONDITION(h <= 100);

      // Now we choose a color some distance betwen "easy" and "hard"
      color = gdImageColorResolve(img.ptr,
                                  intensity(easy[0], hard[0], h),
                                  intensity(easy[1], hard[1], h),
                                  intensity(easy[2], hard[2], h));
    }

    gdImageFilledPolygon(img.ptr, points, 4, color);

    prevX = x;
    prevY = y;
  }
}

void graphElevation(const Image& img, const Track& track,
                    int start, int end, int color) {
  PRECONDITION(start >= 0);
  PRECONDITION(start < track.size());
  PRECONDITION(end >= start);
  PRECONDITION(end <= track.size());

  gdImageSetThickness(img.ptr, 2);
  gdImageSetAntiAliased(img.ptr, color);

  int prev_x = -1;
  int prev_y = -1;

  int point = start;
  while (point <= end) {
    const int x = img.getX(track[point].length);
    // Determine the average elevation of all points that map to x
    double running_elevation = track[point].elevation;
    int elevation_points = 1;
    ++point;
    while (point <= end && img.getX(track[point].length) == x) {
      running_elevation += track[point].elevation;
      ++elevation_points;
      ++point;
    }
    const int y = img.getY(running_elevation / elevation_points);

    if (prev_x >= 0) {
      gdImageLine(img.ptr, prev_x, prev_y, x, y, gdAntiAliased);
    }
    prev_x = x;
    prev_y = y;
  }
}

// Boundaries for rendered text are given as an array of 8 integers, or
// four (x,y) points: lower left, lower right, upper right, upper left.
const int LLX_INDEX = 0;
const int LLY_INDEX = 1;
const int LRX_INDEX = 2;
const int LRY_INDEX = 3;
const int URX_INDEX = 4;
const int URY_INDEX = 5;
const int ULX_INDEX = 6;
const int ULY_INDEX = 7;

void drawTicsY(const Image & img) {
  const int grey = gdImageColorResolve(img.ptr, 0xD0, 0xF0, 0xF4);
  const int semi = gdImageColorResolve(img.ptr, 0xD8, 0xF8, 0xF8);
  const int dark = gdImageColorResolve(img.ptr, 0x40, 0x40, 0x40);

  const double pt = img.options.pointSize;

  // Draw the minor tic marks
  for (double ele = img.min; ele < img.max ; ele += img.subIncr) {
    const int y = img.getRawY(ele);
    gdImageLine(img.ptr, img.left, y, img.right, y, semi);
  }

  // Boundaries of the rendered text, as described above.
  int rect[8];

  // Draw the major tic marks, and the tic labels
  for (double ele = img.min; ele <= img.max ; ele += img.incr) {
    // Don't draw the top or bottom line, because the two-pixel line
    // will be outside the border
    const int y = img.getRawY(ele);
    if (ele > img.min && ele < img.max) {
      gdImageSetThickness(img.ptr, 2);
      gdImageLine(img.ptr, img.left, y, img.right, y, grey);
      gdImageSetThickness(img.ptr, 1);
    }

    // Draw the tic label
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "%g", ele);

    gdImageStringFT(nullptr, rect, dark, img.font(), pt, 0, 0, 0, buffer);
  
    gdImageStringFT(img.ptr, 0, dark, img.font(), pt, 0,
                    (img.left - rect[LRX_INDEX]) - 5,
                    y - rect[URY_INDEX] / 2,
                    buffer);

    // Draw a little tab next to the label, in the same color as the label.
    gdImageLine(img.ptr, img.left, y, img.left - 3, y, dark);
  }

  // Write the Y label, rotated 90 degrees (aka PI/2).
  const char* label = "elevation (feet)";
  if (img.options.metric) {
    label = "elevation (meters)";
  }

  gdImageStringFT(nullptr, rect, 0, img.font(), pt, PI/2,
                  0, 0, const_cast<char*>(label));

  gdImageStringFT(img.ptr, 0, dark, img.font(), pt, PI/2,
                  rect[LLX_INDEX] - rect[URX_INDEX],
                  (img.bottom + img.top) / 2 - rect[URY_INDEX]/2,
                  const_cast<char*>(label));
}

void drawTicsX(const Image & img) {
  const double pt = img.options.pointSize;

  const double length = img.distance(img.length);  // in display units
  double incr = pow(10, floor(log10(length)));
  const double tics = length / incr;

  if (tics > 8) {
    incr *= 2;
  } else if (tics < 4) {
    incr /= 2;
  }

  const int dark = gdImageColorResolve(img.ptr, 0x40, 0x40, 0x40);
  int rect[8];

  // Track the lowest tic label
  int ticBottom = 0;

  for (double tic = incr; tic < length; tic += incr) {
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "%g", tic);

    gdImageStringFT(nullptr, rect, dark, img.font(), pt, 0, 0, 0, buffer);

    const int x = (tic / length) * (img.right - img.left) + img.left;
    const int y = img.bottom + 6 - rect[URY_INDEX];
    if (y > ticBottom) ticBottom = y;
 
    // Just draw the little tab below the image.
    gdImageLine(img.ptr, x, img.bottom, x, img.bottom + 5, dark);

    gdImageStringFT(img.ptr, 0, dark, img.font(), pt, 0,
                    x - rect[LRX_INDEX] / 2, y, buffer);
  }

  const char* label = "distance (miles)";
  if (img.options.metric) {
    label = "distance (kilometers)";
  }

  gdImageStringFT(nullptr, rect, dark, img.font(), pt, 0, 0, 0,
                  const_cast<char*>(label));

  gdImageStringFT(img.ptr, 0, dark, img.font(), pt, 0,
                  (img.right + img.left) / 2 - rect[LRX_INDEX] / 2,
                  ticBottom + 5 - rect[URY_INDEX],
                  const_cast<char*>(label));
}

// Mark the most difficult km, if there was one, as a background rectangle.
void drawDifficult(const Image& img, const Track& track) {
  int start = 0;
  int end = 0;
  double score = 0;
  track.mostDifficult(1000, start, end, score);

  if (score > 0) {
    const int left_x = img.getX(track[start].length);
    const int right_x = img.getX(track[end].length);

    const int color = gdImageColorResolveAlpha(img.ptr, 0xF0, 0xA0, 0xA0, 0x60);
    gdImageFilledRectangle(img.ptr, left_x, img.top, right_x, img.bottom,
                           color);
  }
}

void calculateDataRange(Image& img, const Track& track) {
  const double min_ele = img.elevation(track.getMinimumElevation());
  const double max_ele = img.elevation(track.getMaximumElevation());

  img.incr = pow(10, floor(log10(max_ele - min_ele)));
  img.subIncr = img.incr / 10;

  double tics = (max_ele - min_ele) / img.incr;
  if (tics > 6) {
    img.incr *= 2;
    img.subIncr *= 2;
  } else if (tics < 3) {
    img.incr /= 2;
    // Leave the subIncr the same -- it looks better.
  }

  img.min = floor(min_ele / img.incr) * img.incr;
  img.max = ceil(max_ele / img.incr) * img.incr;
  img.length = track.getTotalDistance();
}

// Calculates where the left edge of the image should start, considering
// the left-side labels
int calculateLeftBorder(const Image& img) {
  int rect[8];
  const double pt = img.options.pointSize;
  int right = 0;

  // Check the length of *each* of the tic labels
  for (int tic = img.min; tic <= img.max; tic += img.incr) {
    char buffer[100];
    snprintf(buffer, sizeof(buffer), "%d", tic);
    gdImageStringFT(0, rect, 0, img.font(), pt, 0, 0, 0, buffer);
    right = std::max(right, rect[LRX_INDEX]);
  }

  gdImageStringFT(nullptr, rect, 0, img.font(), pt, PI/2,
                  0, 0, const_cast<char*>("elevation (meters)"));

  POSTCONDITION(rect[URX_INDEX] < 0);
  // Length of longest tic + height of label + buffers on both sides
  return right + (rect[LLX_INDEX] - rect[URX_INDEX]) + 5 + 5;
}

int calculateBottomBorder(const Image& img) {
  int rect[8];
  const double pt = img.options.pointSize;

  // This is just a sample, assuming all the distance labels would be similar.
  char* text = const_cast<char*>("10");

  gdImageStringFT(nullptr, rect, 0, img.font(), pt, 0, 0, 0, text);
  POSTCONDITION(rect[URY_INDEX] < 0);

  const int border = rect[LLY_INDEX] - rect[URY_INDEX];

  text = const_cast<char*>("distance (miles)");
  gdImageStringFT(nullptr, rect, 0, img.font(), pt, 0, 0, 0, text);

  // tic height + label height + buffers on both sides
  return border + 5 + (rect[LLY_INDEX] - rect[URY_INDEX]) + 5;
}

}  // namespace

void PNG::write(ostream& out, const Track& track, Options opt) {
  Image img(opt);

  const int white = gdImageColorResolve(img.ptr, 0xFF, 0xFF, 0xFF);
  gdImageFilledRectangle(img.ptr,
                         0, 0, img.options.width - 1, img.options.height - 1,
                         white);

  // Fill in the min/max/etc Y data values
  calculateDataRange(img, track);

  img.left = calculateLeftBorder(img);  // account for labels, etc.
  img.right = img.options.width - 10;   // 10 pixel border
  img.top = 10;                         // 10 pixel border
  img.bottom = img.options.height - calculateBottomBorder(img);

  // Draw the background of the graph
  const int background = gdImageColorResolve(img.ptr, 0xF0, 0xFF, 0xFF);
  gdImageFilledRectangle(img.ptr,
                         img.left, img.top, img.right, img.bottom,
                         background);

  drawTicsY(img);
  drawTicsX(img);

  if (opt.climbs) {
    const int light = gdImageColorResolveAlpha(img.ptr, 0xB0, 0xD0, 0xB0, 0x60);

    // First, draw the climb markers and background
    for (const Track::Climb& climb : track.getClimbs()) {
      const int leftX = img.getX(climb.getStart().length);
      const int rightX = img.getX(climb.getEnd().length);

      // Draw a rectangle all the way down the graph to show the area
      gdImageFilledRectangle(img.ptr,
                             leftX, img.top, rightX, img.bottom,
                             light);
    }
  }

  if (opt.difficult) {
    drawDifficult(img, track);
  }

  graphGrade(img, track, opt.grade);

  const int elevationLine = gdImageColorResolve(img.ptr, 0x30, 0x50, 0x30);
  graphElevation(img, track, 0, track.size() - 1, elevationLine);

  if (opt.climbs) {
    // Above we marked the climbs with an area "behind" the grade. In this
    // section we draw the text, and a little bar at the bottom.
    const int textColor = gdImageColorResolve(img.ptr, 0x40, 0x40, 0x40);
    const int strip = gdImageColorResolve(img.ptr, 0x10, 0x50, 0x10);

    // Draw the text (length & grade)
    for (const Track::Climb& climb : track.getClimbs()) {
      const int left_x = img.getX(climb.getStart().length);
      const int right_x = img.getX(climb.getEnd().length);

      const double len = climb.getEnd().length - climb.getStart().length;
      const double grade =
          ((climb.getEnd().elevation - climb.getStart().elevation) /
           len) * 100.0;

      // TODO: Center both of the lines, individually
      char buffer[100];
      snprintf(buffer, sizeof(buffer), "%0.1f%s\n %0.1f%%",
               img.distance(len),
               (img.options.metric ? "km" : "m"),
               grade);

      int rect[8];
      gdImageStringFT(nullptr, rect, 0, img.font(),
                      8, 0, 0, 0, buffer);

      // Mid-point of the climb - half width of the text
      const int x =
          (right_x + left_x) / 2 - (rect[LRX_INDEX] - rect[ULX_INDEX]) / 2;
      // top + half height of text + 5 pixel buffer
      const int y = img.top + (rect[LLY_INDEX] - rect[URY_INDEX]) / 2 + 5;

      gdImageStringFT(img.ptr, 0, textColor, img.font(), 8, 0, x, y, buffer);

      // Draw a bar at the bottom of the image to more clearly mark
      // the start/end of a climb
      gdImageFilledRectangle(img.ptr,
                             left_x, img.bottom - 4, right_x, img.bottom,
                             strip);
    }
  }

  // Draw a border
  gdImageSetThickness(img.ptr, 1);
  int grey  = gdImageColorResolve(img.ptr, 0x80, 0x80, 0x80);
  gdImageRectangle(img.ptr, img.left, img.top, img.right, img.bottom, grey);

  // Write to 'out'
  gdSink sink;
  sink.context = &out;
  sink.sink = writeTo;

  gdImagePngToSink(img.ptr, &sink);
}
