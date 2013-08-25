
LIBSRC := point.cc track.cc gpx.cc document.cc fit.cc png.cc json.cc \
	  dir.cc kml.cc gnuplot.cc util.cc text.cc parse.cc
LIBOBJ := $(LIBSRC:.cc=.o)

TSTSRC := trackmain.cc sameroute.cc
TSTOBJ := $(TSTSRC:.cc=.o)
TSTBIN := $(TSTSRC:.cc=)

LIB    := libtrack.a

CXXFLAGS := -g -fPIC

LDFLAGS += -L. -ltrack -lfit -lgd -LLIBDIR -L/usr/local/lib


all: lib track sameroute

lib: $(LIB)

$(LIB): $(LIBOBJ)
#	libtool -static $(LIBOBJ) -o $(LIB)
	ar rcsu $(LIB) $(LIBOBJ)

track: $(LIB) trackmain.o
	$(CXX) trackmain.o -o track $(LDFLAGS)

sameroute: $(LIB) sameroute.o
	$(CXX) sameroute.o -o sameroute $(LDFLAGS)

clean:
	-$(RM) $(LIBOBJ) $(TSTSRC:.cc=.o) $(TSTBIN) $(LIB) *~

