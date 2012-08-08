#include "png.h"
#include "track.h"
#include "exception.h"

#include <iostream>
#include <stdio.h>
#include <math.h>

#include "gd.h"
#include "gdfontl.h"

using namespace std;

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

    char * font() { return const_cast<char*>(options.font.c_str()); }

    double distance(double d) const {
        if (options.metric) {
            return d / 1000.0;
        } else {
            return (d / 1000.0) * 0.62137;
        }
    }

    double elevation(double d) const {
        if (options.metric) {
            return d;
        } else {
            return d * 3.2808;
        }
    }

    // borders of the graph portion of the image, inside the labels
    int top, left, bottom, right;

    gdImagePtr ptr;

    PNG::Options options;

    // Max and min Y values, in terms of the displayed data (not the source
    // data in meters, not the pixels). So if it's metric, it's in meters,
    // otherwise in feet.

    double min;
    double max;
    double incr;
    double subIncr;
    double length;

    // Return the X position (in pixels) of a given data value
    int getX(double pos) {
        return (pos / length) * (right - left) + left;
    }

    // Return the Y position for a value already converted to display form
    // (ie metric or imperial). For example, a value based on 'min' or 'max'.
    int getRawY(double ele) {
        return bottom - (ele - min) / (max - min) * (bottom - top);
    }

    // Return the Y position (in pixels) of a given data value
    int getY(double ele) {
        return getRawY(elevation(ele));
    }
};

static int writeTo(void * context, const char * buffer, int len) {

    PRECONDITION(context != 0);
    PRECONDITION(buffer != 0);

    ostream * out = (ostream *) context;
    out->write(buffer, len);
    return len;
}

static int intensity(int easy, int hard, double actual) {
    return (int) (easy * (1 - actual) + hard * actual);
}

static void graphGrade(Image & img, Track & track, bool grade) {

    if (track.empty()) return;

    double len = track.getTotalDistance();

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

static void graphElevation(Image & img, Track & track,
                           int start, int end, int color) {

    PRECONDITION(start >= 0);
    PRECONDITION(start < track.size());
    PRECONDITION(end >= start);
    PRECONDITION(end <= track.size());

    int prevX = img.getX(track[start].length);
    int prevY = img.getY(track[start].elevation);

    gdImageSetThickness(img.ptr, 2);

    for (int i = start; i <= end; i++) {

        int x = img.getX(track[i].length);
        int y = img.getY(track[i].elevation);

        if ((x != prevX) || (y != prevY)) {
            gdImageLine(img.ptr, prevX, prevY, x, y, color);

            prevX = x;
            prevY = y;
        }
    }
}


static void drawTicsY(Image & img) {

    int grey = gdImageColorResolve(img.ptr, 0xD0, 0xF0, 0xF4);
    int semi = gdImageColorResolve(img.ptr, 0xD8, 0xF8, 0xF8);
    int dark = gdImageColorResolve(img.ptr, 0x40, 0x40, 0x40);

    double pt = img.options.pointSize;

    int rect[8];

    // Draw the minor tic marks
    for (double ele = img.min; ele < img.max ; ele += img.subIncr) {

        int y = img.getRawY(ele);
        gdImageLine(img.ptr, img.left, y, img.right, y, semi);
    }

    // Draw the major tic marks, and the tic labels
    for (double ele = img.min; ele <= img.max ; ele += img.incr) {

        // Don't draw the top or bottom line, because the two-pixel line
        // will be outside the border
        int y = img.getRawY(ele);
        if ((ele > img.min) && (ele < img.max)) {
            gdImageSetThickness(img.ptr, 2);
            gdImageLine(img.ptr, img.left, y, img.right, y, grey);
            gdImageSetThickness(img.ptr, 1);
        }

        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%g", ele);
        gdImageStringFT(0, &rect[0], dark, img.font(), pt, 0, 0, 0, buffer);

        gdImageStringFT(img.ptr, 0, dark, img.font(), pt, 0,
                        (img.left - rect[2]) - 5,
                        y - rect[5] / 2,
                        buffer);

        gdImageLine(img.ptr, img.left, y, img.left - 3, y, dark);
    }

    // Write the base label

    const char * label = "elevation (feet)";
    if (img.options.metric) {
        label = "elevation (meters)";
    }

    gdImageStringFT(0, &rect[0], 0, img.font(), pt, 3.14159/2,
                    0, 0, const_cast<char*>(label));

    gdImageStringFT(img.ptr, 0, dark, img.font(), pt, 3.14159/2,
                    -(rect[4] - rect[0]),
                    (img.bottom + img.top) / 2 - rect[5]/2,
                    const_cast<char*>(label));
}

static void drawTicsX(Image & img) {

    double pt = img.options.pointSize;

    double length = img.distance(img.length); // track distance in display units
    double incr = pow(10, floor(log10(length)));
    double tics = length / incr;

    if (tics > 8) {
        incr *= 2;
    } else if (tics < 4) {
        incr /= 2;
    }

    int dark = gdImageColorResolve(img.ptr, 0x40, 0x40, 0x40);
    int rect[8];

    // Track the lowest tic label
    int ticBottom = 0;

    for (double tic = incr; tic < length; tic += incr) {

        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%g", tic);

        gdImageStringFT(0, &rect[0], dark, img.font(), pt, 0,
                        0, 0, buffer);

        int x = (tic / length) * (img.right - img.left) + img.left;
 
        gdImageLine(img.ptr, x, img.bottom, x, img.bottom + 5, dark);

        int y = img.bottom + 6 - rect[5];
        if (y > ticBottom) ticBottom = y;

        gdImageStringFT(img.ptr, 0, dark, img.font(), pt, 0,
                        x - rect[2] / 2, y, buffer);
    }

    const char * label = "distance (miles)";
    if (img.options.metric) {
        label = "distance (kilometers)";
    }

    gdImageStringFT(0, rect, dark, img.font(), pt, 0, 0, 0,
                    const_cast<char*>(label));

    gdImageStringFT(img.ptr, 0, dark, img.font(), pt, 0,
                    (img.right + img.left) / 2 - rect[2] / 2,
                    ticBottom + 5 - rect[5],
                    const_cast<char*>(label));
}

static void drawDifficult(Image & img, Track & track) {

    int start = 0;
    int end = 0;
    double score = 0;
    track.mostDifficult(1000, start, end, score);

    if (score > 0) {

        int leftX = img.getX(track[start].length);
        int rightX = img.getX(track[end].length);

        int color = gdImageColorResolveAlpha(img.ptr, 0xF0, 0xA0, 0xA0, 0x60);
        gdImageFilledRectangle(img.ptr,
                               leftX, img.top, rightX, img.bottom,
                               color);
    }
}

static void calculateDataRange(Image & img, Track & track) {

    img.min = img.elevation(track.getMinimumElevation());
    img.max = img.elevation(track.getMaximumElevation());

    img.incr = pow(10, floor(log10(img.max - img.min)));
    img.subIncr = img.incr / 10;

    img.length = track.getTotalDistance();

    double tics = (img.max - img.min) / img.incr;
    if (tics > 6) {
        img.incr *= 2;
        img.subIncr *= 2;
    } else if (tics < 3) {
        img.incr /= 2;
    }
    
    img.min = floor(img.min / img.incr) * img.incr;
    img.max = ceil(img.max / img.incr) * img.incr;
}

static void writeRect(int * rect) {
    cerr << rect[0] << "," << rect[1] << endl
         << rect[2] << "," << rect[3] << endl
         << rect[4] << "," << rect[5] << endl
         << rect[5] << "," << rect[6] << endl;
}

// Calculate where the left edge of the image should start, considering
// the left-side labels
static int calculateLeftBorder(Image & img) {

    int rect[8];
    double pt = img.options.pointSize;
    int right = 0;

    // Check the length of *each* of the tic labels
    for (int tic = img.min; tic <= img.max; tic += img.incr) {

        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%d", tic);
        gdImageStringFT(0, rect, 0, img.font(), pt, 0, 0, 0, buffer);

        if (rect[2] > right) right = rect[2];
    }

    gdImageStringFT(0, rect, 0, img.font(), pt, 3.14159/2,
                    0, 0, (char *) "elevation (meters)");
    POSTCONDITION(rect[4] < 0);

    return right + -(rect[4] - rect[0]) + 5 + 5;
}

static int calculateBottomBorder(Image & img) {

    int rect[8];
    double pt = img.options.pointSize;

    // The text here are just samples
    char * text = const_cast<char*>("10");
    char * font = const_cast<char*>("helvetica");

    gdImageStringFT(0, &rect[0], 0, font, pt, 0, 0, 0, text);
    POSTCONDITION(rect[5] < 0);

    int border = rect[1] - rect[5];

    text = const_cast<char*>("distance (miles)");
    gdImageStringFT(0, &rect[0], 0, font, pt, 0, 0, 0, text);

    return border + 5 + (rect[1] - rect[5]) + 5;
}

void PNG::write(ostream & out, Track & track, Options opt) {

    Image img(opt);

    int white = gdImageColorResolve(img.ptr, 0xFF, 0xFF, 0xFF);
    gdImageFilledRectangle(img.ptr,
                           0, 0, img.options.width-1, img.options.height-1,
                           white);

    // Fill in the min/max/etc Y data values
    calculateDataRange(img, track);

    // Start with a 10-pixel border around the whole image
    img.left = 10;
    img.right = img.options.width - 10;
    img.top = 10;
    img.bottom = img.options.height - 10;

    // Reset the left & bottom to account for labels, etc.
    img.left = calculateLeftBorder(img);
    img.bottom = img.options.height - calculateBottomBorder(img);

    {
        // Draw the background of the graph
        int background = gdImageColorResolve(img.ptr, 0xF0, 0xFF, 0xFF);
        gdImageFilledRectangle(img.ptr,
                               img.left, img.top, img.right, img.bottom,
                               background);
    }

    drawTicsY(img);
    drawTicsX(img);

    if (opt.climbs) {

        int lighter = gdImageColorResolveAlpha(img.ptr, 0xB0, 0xD0, 0xB0, 0x60);

        // First, draw the climb markers and background

        for (int i = 0; i < track.getClimbs().size(); i++) {

            const Track::Climb & climb(track.getClimbs()[i]);

            int leftX = img.getX(climb.getStart().length);
            int rightX = img.getX(climb.getEnd().length);


            // Draw a rectangle all the way down the graph to show the area
            gdImageFilledRectangle(img.ptr,
                                   leftX, img.top, rightX, img.bottom,
                                   lighter);
        }
    }

    if (opt.difficult) {
        drawDifficult(img, track);
    }

    graphGrade(img, track, opt.grade);

    {
        int elevationLine = gdImageColorResolve(img.ptr, 0x30, 0x50, 0x30);
        graphElevation(img, track, 0, track.size()-1, elevationLine);
    }

    if (opt.climbs) {

        int textColor = gdImageColorResolve(img.ptr, 0x40, 0x40, 0x40);
        int strip = gdImageColorResolve(img.ptr, 0x10, 0x50, 0x10);

        // Now, draw the text (length & grade)
        for (int i = 0; i < track.getClimbs().size(); i++) {

            const Track::Climb & climb(track.getClimbs()[i]);

            int leftX = img.getX(climb.getStart().length);
            int rightX = img.getX(climb.getEnd().length);

            double len = climb.getEnd().length - climb.getStart().length;
            double grade = ((climb.getEnd().elevation - climb.getStart().elevation) / len) * 100.0;

            // TODO: Actually center the two lines
            char buffer[100];
            snprintf(buffer, sizeof(buffer), "%0.1f%s\n %0.1f%%",
                     img.distance(len),
                     (img.options.metric ? "km" : "m"),
                     grade);

            int rect[8];
            gdImageStringFT(0, rect, 0, const_cast<char*>(opt.font.c_str()),
                            8, 0, 0, 0, buffer);

            gdImageStringFT(img.ptr, 0, textColor,
                            const_cast<char*>(opt.font.c_str()),
                            8, 0,
                            (rightX + leftX) / 2 - rect[2] / 2,
                            img.top - (rect[5] - rect[1]),
                            buffer);

            // Draw a bar at the bottom of the image to more clearly mark
            // the start/end of a climb
            gdImageFilledRectangle(img.ptr,
                                   leftX, img.bottom - 4, rightX, img.bottom,
                                   strip);

        }
    }

    // Draw a border
    gdImageSetThickness(img.ptr, 1);
    int grey  = gdImageColorResolve(img.ptr, 0x80, 0x80, 0x80);
    gdImageRectangle(img.ptr, img.left, img.top, img.right, img.bottom, grey);

    // Write to stdout
    gdSink sink;
    sink.context = &out;
    sink.sink = writeTo;

    gdImagePngToSink(img.ptr, &sink);
}
