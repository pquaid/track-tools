#include "gnuplot.h"
#include "track.h"
#include "exception.h"

using namespace std;

static const char * xaxis(bool metric) {
  // X-axis value is meters, which we want to display as either
  // KM or miles. KM is ($1/1000.0), miles is (($1/1000)*0.62137)

  if (metric) {
    return "($1/1000.0)";
  } else {
    return "(($1/1000.0)*0.62137)";
  }
}

static const char * yaxis(bool metric) {
  if (metric) {
    return "($2)";
  } else {
    return "($2*3.2808)";
  }
}

static string axes(bool metric) {
  return string(xaxis(metric)) + ":" + yaxis(metric);
}

void Gnuplot::write(ostream & out, Track & track, Options options) {
  PRECONDITION(options.terminal != 0);

  // Is there a 'difficult' section of the ride?
  int start = 0;
  int end = 0;
  double score = 0;
  track.mostDifficult(1000, start, end, score);
  bool difficult = options.difficult && (start >= 0);

  out << "set grid" << endl
      << "set y2tics" << endl
      << "set xlabel 'Distance "
      << (options.metric ? "(km)'" : "(miles)'") << endl
      << "set ylabel 'Elevation "
      << (options.metric ? "(meters)'" : "(feet)'") << endl
      << "set y2label 'Grade (%)'" << endl
      << "set terminal " << options.terminal << endl
      << "set style line 1 linewidth 2 linecolor rgb '#60b060'" << endl
      << "set style line 2 linewidth 1 linecolor rgb '#b06060'" << endl
      << "set style line 3 linewidth 2 linecolor rgb '#707030'" << endl
      << "set style line 4 linewidth 2 linecolor rgb '#a02020'" << endl
      << "set object 1 rectangle from graph 0, graph 0 to"
      << " graph 1, graph 1 behind"
      << " fillcolor rgb '#f0ffff' fillstyle solid 1.0" << endl;

  const char * m_to_m = "*0.62137";
  const char * m_to_f = "*3.2808";
  if (options.metric) {
    m_to_m = "";
    m_to_f = "";
  }

  out << "plot ";
  bool plotted = false;

  if (options.grade) {
    out << (plotted ? "," : "");
    plotted = true;

    // Show the grade, but mask out negative grades
    out << "'-' using " << xaxis(options.metric) << ":(($2 > 0) ? $2 : 0)"
        << " with filledcurves above x1 fill solid 0.6 border linestyle 2"
        << " axis x1y2 title 'Grade (%)'";
  }

  if (options.elevation) {
    out << (plotted ? "," : "");
    plotted = true;

    out << "'-' using " << axes(options.metric)
        << " with filledcurves above x1 fill transparent solid 0.8"
        << " border linestyle 1 title 'Elevation'";
  }

  // Highlight each of the climbs
  if (options.climbs && !track.getClimbs().empty()) {
    for (int i = 0; i < track.getClimbs().size(); i++) {
      out << (plotted ? "," : "");
      plotted = true;

      out << "'-' using " << axes(options.metric)
          << " with lines linestyle 3 notitle";
    }
  }

  // Highlight the most difficult KM
  if (options.difficult && difficult) {
    out << (plotted ? "," : "");
    plotted = true;

    out << "'-' using " << axes(options.metric)
        << " with lines linestyle 4 notitle";
  }
  out << endl;

  // Now write all the data

  streamsize prev = out.precision();
  out.precision(8);

  if (options.grade) {
    for (int i = 0; i < track.size(); i++) {
      out << track[i].length << " " << track[i].grade << endl;
    }
    out << "e" << endl;
  }

  if (options.elevation) {
    for (int i = 0; i < track.size(); i++) {
      out << track[i].length << " " << track[i].elevation << endl;
    }
    out << "e" << endl;
  }

  if (options.climbs) {
    for (int i = 0; i < track.getClimbs().size(); i++) {
      const Track::Climb & climb(track.getClimbs()[i]);
      for (int j = climb.getStart().seq; j < climb.getEnd().seq; j++) {
        out << track[j].length << " " << track[j].elevation << endl;
      }
      out << "e" << endl;
    }
  }

  if (options.difficult && difficult) {
    for (int i = start; i <= end; i++) {
      out << track[i].length << " " << track[i].elevation << endl;
    }
    out << "e" << endl;
  }

  out.precision(prev);
}
