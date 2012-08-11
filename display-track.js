/*
  Display track data in an HTML canvas.

  TODO: The whole point of having an HTML graph is to make it
  interactive, but so far this isn't interactive at all.

  This uses the JS output from track tools, which creates an object
  called 'trackData', containing arrays of points and climbs, and
  the most difficult kilometer of the ride.

  Example output from 'track <something> -o json':

  {
    "points": [
    [13523.1,201.932],
    [44060.7,171.302],
    [78010.2,163.604],
    [100480,275.89],
    [121471,300.916]],
    "climbs": [
    { "start": 2, "end": 3 }],
    "difficult": { "start": 4, "end": 4, "score": 47.8583 }
    }

  "points" is an array of [distance,elevation] values, both expressed
  in meters.

  "climbs" is an array of start/end points of automatically-detected
  climbs, expressed as indexes within the 'points' array. Note
  that 'end' is part of the climb.

  "difficult" indicates the most difficult kilometer of the ride, again
  expressed as indexes. The score is an arbitrary number, but scores <= 0
  mean there was no climb detected.

  html-example.html shows how to use this.
*/


function getLabelWidthY(ctx, img) {

    ctx.font = '10pt sans-serif';
    var width = 0;
    for (var i = img.minY; i <= img.maxY; i += img.incrY) {
        var w = ctx.measureText(i + "").width;
        if (w > width) width = w;
    }

    return width;
}

function displayTicsY(ctx, img) {

    var width = getLabelWidthY(ctx, img);
    ctx.fillStyle = '#404040';
    ctx.font = '10pt sans-serif';
    for (var i = img.minY; i <= img.maxY; i += img.incrY) {

        var w = ctx.measureText(i + "").width;
        var y = Math.round(img.getRawY(i)) + 0.5;

        ctx.fillText(i + "", (width - w)+5, y+4);
        
        ctx.beginPath();
        ctx.lineWidth = 1;
        ctx.strokeStyle = '#c0c0c0';
        ctx.moveTo(img.left, y);
        ctx.lineTo(img.right, y);
        ctx.stroke();
    }
}

function displayTicsX(ctx, img) {

    ctx.fillStyle = '#404040';
    ctx.font = '10pt sans-serif';
    for (var i = (img.minX + img.incrX); i <= img.maxX; i += img.incrX) {

        var v = Math.round(i - img.minX);
        var w = ctx.measureText(v + "").width;
        var x = img.getRawX(i);
        ctx.fillText(v + "", x - w/2, img.bottom + 12);
    }

    var label = "distance (km)";
    if (!img.metric) {
        label = "distance (miles)";
    }
    var w = ctx.measureText(label).width;
    ctx.fillText(label, img.width / 2 - w/2, img.bottom + 24);
}

function calculateDataRange(track) {

    // First find the min/max data values
    track.minX = track.maxX = track.points[0][0];
    track.minY = track.maxY = track.points[0][1];

    for (var i in track.points) {
        var x = track.points[i][0];
        var y = track.points[i][1];
        if (x > track.maxX) track.maxX = x;
        if (x < track.minX) track.minX = x;
        if (y > track.maxY) track.maxY = y;
        if (y < track.minY) track.minY = y;
    }

    // These numbers need to be in terms of the displayed values, to make
    // the tics reasonable.

    track.minX = track.distance(track.minX);
    track.maxX = track.distance(track.maxX);
    track.minY = track.elevation(track.minY);
    track.maxY = track.elevation(track.maxY);

    // Now figure out the appropriate tic increments
    var range = track.maxY - track.minY;
    if (range == 0) {
        track.incrY = 1;
    } else {

        track.incrY = Math.pow(10, Math.floor(Math.log(range)/Math.LN10));

        var tics = range / track.incrY;
        if (tics < 3) {
            track.incrY /= 2;
        } else if (tics > 7) {
            track.incrY *= 2;
        }
    }

    // We'll adjust the Y min/max to neatly fit the increments. We won't
    // do this with the X values. That's just a display preference; the
    // Y values look better with buffer, and X is better stretched end to end.

    track.minY = Math.floor(track.minY / track.incrY) * track.incrY;
    track.maxY = Math.ceil(track.maxY / track.incrY) * track.incrY;

    range = track.maxX - track.minX;
    if (range == 0) {
        track.incrX = 1;
        track.minX -= 1;
        track.maxX += 1;
    } else {

        track.incrX = Math.pow(10, Math.floor(Math.log(range)/Math.LN10));

        tics = range / track.incrX;
        if (tics < 3) {
            track.incrX /= 2;
        } else if (tics > 7) {
            track.incrX *= 2;
        }
    }
}

function TrackData(inPoints, inClimbs, inDifficult, inMetric) {

    this.points    = inPoints;
    this.climbs    = inClimbs;
    this.difficult = inDifficult;
    this.metric    = inMetric;

    this.minX  = 0;
    this.maxX  = 0;
    this.minY  = 0;
    this.maxY  = 0;
    this.incrX = 0;
    this.incrY = 0;

    // These really pertain to the resulting graph, not the track itself
    this.left   = 0;
    this.right  = 0;
    this.top    = 0;
    this.bottom = 0;
    this.height = 0;
    this.width  = 0;

    // Return the presentable value, ie metric or not.
    this.elevation = function(val) {
        if (this.metric) {
            return val;
        } else {
            return val * 3.2808;
        }
    }
    
    this.distance = function(val) {
        if (this.metric) {
            return val / 1000;
        } else {
            return (val / 1000) * 0.62137;
        }
    }

    // Calculate the X or Y pixel position of the given data value, expressed
    // in presentation form (ie metric or not).

    this.getRawY = function(val) {
        return this.bottom - ((val - this.minY) / (this.maxY - this.minY)) * (this.bottom - this.top);
    }

    this.getRawX = function(val) {
        return ((val - this.minX) / (this.maxX - this.minX)) * (this.right - this.left) + this.left;
    }

    // Calculate the X or Y pixel position of the given point within the
    // 'points' array.

    this.getY = function(idx) {
        return this.getRawY(this.elevation(this.points[idx][1]));
    }

    this.getX = function(idx) {
        return this.getRawX(this.distance(this.points[idx][0]));
    }   

}

function trackToolsDisplay(canvasId, track) {

    var canvas = document.getElementById(canvasId);
    var ctx = canvas.getContext('2d');

    if (track.points.length == 0) return;

    calculateDataRange(track);

    track.width = canvas.width;
    track.height = canvas.height;

    track.left   = 10 + getLabelWidthY(ctx, track); // make room for labels
    track.right  = track.width - 10;
    track.top    = 10;
    track.bottom = track.height - 30; // allow room for labels

    ctx.fillStyle = "#f0ffff";
    ctx.fillRect(track.left, track.top,
                 track.right - track.left,
                 track.bottom - track.top);

    displayTicsY(ctx, track);
    displayTicsX(ctx, track);

    // Draw the elevation background gradient

    gradient = ctx.createLinearGradient(0, 0, 0, track.bottom);
    gradient.addColorStop("0", 'rgba(236,255,236,0.6)');
    gradient.addColorStop("1.0", 'rgba(32,72,32,1)');

    ctx.beginPath();
    ctx.fillStyle = gradient;
    ctx.moveTo(track.getX(0), track.bottom);
    ctx.lineTo(track.getX(0), track.getY(0));

    for (var i in track.points) {
        ctx.lineTo(track.getX(i), track.getY(i));
    }

    ctx.lineTo(track.getX(track.points.length-1), track.bottom);
    ctx.lineTo(track.getX(0), track.bottom);
    ctx.fill();

    // Draw the elevation line

    ctx.beginPath();
    ctx.strokeStyle = "#306030";
    ctx.lineWidth = 4;
    ctx.lineJoin = 'round';
    
    ctx.moveTo(track.getX(0), track.getY(0));

    for (var i in track.points) {
        ctx.lineTo(track.getX(i), track.getY(i));
    }

    ctx.stroke();

    // Highlight the climbs

    if (track.climbs && (track.climbs.length > 0)) {
        for (var i in track.climbs) {

            ctx.beginPath();
            ctx.strokeStyle = '#a05050';
            ctx.lineWidth = 6;
            ctx.lineJoin = 'round';

            var start = track.climbs[i].start;
            var end = track.climbs[i].end;

            ctx.moveTo(track.getX(start), track.getY(start));

            for (var p = start + 1; p <= end; p++) {
                ctx.lineTo(track.getX(p), track.getY(p));
            }
            ctx.stroke();
        }
    }

    // Indicate the most difficult kilometer

    if (track.difficult && track.difficult.score > 0) {

        var start = track.getX(track.difficult.start);
        var end   = track.getX(track.difficult.end);

        ctx.fillStyle = 'rgba(200,160,160,0.5)';
        ctx.fillRect(start,
                     track.top,
                     end - start,
                     track.bottom - track.top);
    }
}

// GPX methods

function deg2rad(deg) {
    return deg * Math.PI / 180;
}

function latLonDistance(lat1,lon1,lat2,lon2) {

    var dlon = deg2rad(lon2 - lon1);
    var dlat = deg2rad(lat2 - lat1);

    var a = Math.pow(Math.sin(dlat / 2), 2) + Math.cos(deg2rad(lat1)) *
        Math.cos(deg2rad(lat2)) *
        Math.pow(Math.sin(dlon / 2), 2);
    return 2 * 6371000 * Math.asin(Math.sqrt(a));
}

function eleFromTrackpoint(trkpt) {
    var ele = trkpt.getElementsByTagName("ele");
    if (ele && (ele.length > 0) && (ele[0].childNodes.length > 0)) {
        return parseFloat(ele[0].childNodes[0].nodeValue);
    } else {
        return 0;
    }
}

function displayTrackDataGPX(canvasId, doc, metric) {

    var trkpt = doc.getElementsByTagName("trkpt");

    if (trkpt.length == 0) return;

    // We need an array of length/elevation pairs, which GPX doesn't
    // provide directly. We'll have to calculate the distance from
    // point to point.
    //
    // We also want climbs and the most difficult KM, but we're not
    // going to calculate that here. It'll have to be optional.

    var points = new Array();
    {
        var prevLat = parseFloat(trkpt[0].getAttribute('lat'));
        var prevLon = parseFloat(trkpt[0].getAttribute('lon'));

        points.push([ 0, eleFromTrackpoint(trkpt[0]) ]);

        var sofar = 0;

        for (var i = 1; i < trkpt.length; i++) {

            var lat = parseFloat(trkpt[i].getAttribute('lat'));
            var lon = parseFloat(trkpt[i].getAttribute('lon'));

            sofar += latLonDistance(prevLat,prevLon,lat,lon);
            points.push([ sofar, eleFromTrackpoint(trkpt[i]) ]);

            prevLat = lat;
            prevLon = lon;
        }
    }

    var track = new TrackData(points,
                              null,
                              null,
                              metric);

    trackToolsDisplay(canvasId, track);
}

function loadTrackDataGPX(canvasId, url, metric) {

    var xmlhttp=new XMLHttpRequest();
    xmlhttp.open("GET", url, false);
    xmlhttp.send();
    displayTrackDataGPX(canvasId, xmlhttp.responseXML, metric);
}

// JSON methods

function displayTrackDataJSON(canvasId, points, metric) {

    var track = new TrackData(points.points,
                              points.climbs,
                              points.difficult,
                              metric);

    trackToolsDisplay(canvasId, track);
}

function loadTrackDataJSON(canvasId, url, metric) {

    var req = new XMLHttpRequest();
    req.open("GET", url, false);
    req.send();
    var obj = JSON.parse(req.responseText);

    displayTrackDataJSON(canvasId, obj, metric);
}


