#include "fit.h"

#include <iosfwd>

#include "libfit/fit_decode.hpp"
#include "libfit/fit_mesg_broadcaster.hpp"

#include <stdint.h>

#include "track.h"

using namespace std;

double toDegree(int32_t semicircles) {
  return (semicircles * -180.0) / (1<<31);
}

time_t toTime(FIT_DATE_TIME s) {
  return s + 631065600;
}

class Listener : public fit::RecordMesgListener {
public:

    Listener(vector<Point> & points_)
        : sequence(0), points(points_)
        {}

    void OnMesg(fit::RecordMesg & msg) {

        Point current;

        FIT_SINT32 pos = msg.GetPositionLat();
        if (pos == FIT_SINT32_INVALID) {
            return;
        } else {
            current.lat = toDegree(pos);
        }

        pos = msg.GetPositionLong();
        if (pos != FIT_SINT32_INVALID) {
            current.lon = toDegree(pos);
        }

        FIT_FLOAT32 ele = msg.GetAltitude();
        if (ele != FIT_FLOAT32_INVALID) {
            current.elevation = ele;
        }

        FIT_DATE_TIME t = msg.GetTimestamp();
        if (t != FIT_DATE_TIME_INVALID) {
            current.timestamp = toTime(t);
        }

        FIT_UINT8 hr = msg.GetHeartRate();
        if (hr != FIT_UINT8_INVALID) {
            current.hr = hr;
        }

        current.length = msg.GetDistance();

        FIT_SINT8 temp = msg.GetTemperature();
        if (temp != FIT_SINT8_INVALID) {
            current.atemp = temp;
        }

        current.seq = sequence++;

        points.push_back(current);
    }

private:

    uint32_t sequence;
    vector<Point> & points;
};

void Fit::read(istream & in, Track & points) {

    fit::MesgBroadcaster messages;
    Listener listener(points);

    messages.AddListener((fit::RecordMesgListener &) listener);

    try {
        messages.Run(in);
    } catch (const fit::RuntimeException & e) {
        throw FitException(e.what());
    }
}
