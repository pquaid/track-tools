
LIBSRC := point.cc track.cc gpx.cc document.cc fit.cc png.cc json.cc \
	  dir.cc kml.cc gnuplot.cc util.cc text.cc parse.cc
LIBOBJ := $(LIBSRC:.cc=.o)
LIBDEPS := $(LIBOBJ:.o=.d)

TSTSRC := trackmain.cc sameroute.cc
TSTOBJ := $(TSTSRC:.cc=.o)
TSTDEPS := $(TSTOBJ:.o=.d)
TSTBIN := $(TSTSRC:.cc=)

LIB    := libtrack.a
BIN    := track sameroute

CXXFLAGS := -g -fPIC -std=c++11

LDFLAGS += -L. -ltrack -lfit -lgd

all: lib bin

lib: $(LIB)

bin: $(BIN)

$(LIB): $(LIBOBJ)
#	libtool -static $(LIBOBJ) -o $(LIB)
	ar rcsu $(LIB) $(LIBOBJ)

track: $(LIB) trackmain.o
	$(CXX) trackmain.o -o track $(LDFLAGS)

sameroute: $(LIB) sameroute.o
	$(CXX) sameroute.o -o sameroute $(LDFLAGS)

clean:
	-$(RM) $(LIBOBJ) $(LIBDEPS) $(TSTOBJ) $(TSTDEPS) $(TSTBIN) $(LIB) $(BIN) *~

%.o: %.cc
	$(CXX) -c -MMD -MP $(CXXFLAGS) $< -o $@

-include $(LIBDEPS)
