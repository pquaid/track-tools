cc_library(
  name = "track-formats",
  srcs = [
    "fit.cc",
    "gnuplot.cc",
    "gpx.cc",
    "json.cc",
    "kml.cc",
    "png.cc",
    "text.cc",
  ],
  hdrs = [
    "fit.h",
    "gnuplot.h",
    "gpx.h",
    "json.h",
    "kml.h",
    "png.h",
    "text.h",
  ],
  deps = [
    ":track-utils",
    "//libfit/cpp:libfit",
  ],
)

cc_library(
  name = "track-lib",
  srcs = [
    "parse.cc",
    "point.cc",
    "track.cc",
  ],
  hdrs = [
    "parse.h",
    "point.h",
    "track.h",
  ],
  deps = [
    ":track-formats",
  ],
)

cc_library(
  name = "track-utils",
  srcs = [
    "dir.cc",
    "document.cc",
    "util.cc",
  ],
  hdrs = [
    "dir.h",
    "document.h",
    "exception.h",
    "util.h",
  ],
)

cc_binary(
  name = "track",
  srcs = ["trackmain.cc"],
  deps = [
    ":track-utils",
    ":track-formats",
    ":track-lib",
  ],
  linkopts = [
    "-lfit",
    "-lgd",
    "-lm",
  ],
)
