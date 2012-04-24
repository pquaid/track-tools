
LIBSRC := point.cc track.cc gpx.cc document.cc fit.cc \
	  dir.cc kml.cc gnuplot.cc util.cc text.cc
LIBOBJ := $(LIBSRC:.cc=.o)

TSTSRC := trackmain.cc
TSTOBJ := $(TSTSRC:.cc=.o)
TSTBIN := $(TSTSRC:.cc=)

LIB    := libtrack.a

CXXFLAGS := -g -fPIC

LDFLAGS += -L. -ltrack -lfit

all: lib track

lib: $(LIB)

$(LIB): $(LIBOBJ)
#	libtool -static $(LIBOBJ) -o $(LIB)
	ar rcsu $(LIB) $(LIBOBJ)

track: $(LIB) trackmain.o
	$(CXX) trackmain.o -o track $(LDFLAGS)

clean:
	-$(RM) $(LIBOBJ) $(TSTSRC:.cc=.o) $(TSTBIN) $(LIB) *~

