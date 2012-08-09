/*
  Display track data in an HTML canvas.

  TODO: The whole point of having an HTML graph is to make it
  interactive, but so far this isn't interactive at all.

  This uses the JS output from track tools, which creates an object
  called 'trackData', containing arrays of points and climbs, and
  the most difficult kilometer of the ride.

  Example output from 'track <something> -o js':

  var trackData = new Object();
  trackData.points = [[13523.1,201.932],
  [44060.7,171.302],
  [78010.2,163.604],
  [100480,275.89],
  [121471,300.916]];
  trackData.climbs = [
  { start: 2, end: 3 }];
  trackData.difficult = { start: 4, end: 4, score: 47.8583};

  trackData.points is an array of [distance,elevation] values, both
  expressed in meters.

  trackData.climbs is an array of start/end points of automatically-
  detected climbs, expressed as indexes within the 'points' array. Note
  that 'end' is part of the climb.

  trackData.difficult indicates the most difficult kilometer of the
  ride, again expressed as indexes. The score is an arbitrary number,
  but scores <= 0 mean there was no climb detected.

  html-example.html shows how to use this.
*/

// Define some methods for 'trackData'

// Convert elevation and distance data to the displayed form (metric or not)

trackData.elevation = function(val) {
    if (this.metric) {
        return val;
    } else {
        return val * 3.2808;
    }
}

trackData.distance = function(val) {
    if (this.metric) {
        return val / 1000;
    } else {
        return (val / 1000) * 0.62137;
    }
}

// Calculate the X or Y pixel position of the given data value, expressed
// in presentation form (ie metric or not).

trackData.getRawY = function(val) {
    return this.bottom - ((val - this.minY) / (this.maxY - this.minY)) * (this.bottom - this.top);
}

trackData.getRawX = function(val) {
    return ((val - this.minX) / (this.maxX - this.minX)) * (this.right - this.left) + this.left;
}

// Calculate the X or Y pixel position of the given point within the
// 'points' array.

trackData.getY = function(idx) {
    return this.getRawY(this.elevation(this.points[idx][1]));
}

trackData.getX = function(idx) {
    return this.getRawX(this.distance(this.points[idx][0]));
}


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
    track.incrY = Math.pow(10, Math.floor(Math.log(range)/Math.LN10));

    var tics = range / track.incrY;
    if (tics < 3) {
        track.incrY /= 2;
    } else if (tics > 7) {
        track.incrY *= 2;
    }

    // We'll adjust the Y min/max to neatly fit the increments. We won't
    // do this with the X values. That's just a display preference; the
    // Y values look better with buffer, and X is better stretched end to end.

    track.minY = Math.floor(track.minY / track.incrY) * track.incrY;
    track.maxY = Math.ceil(track.maxY / track.incrY) * track.incrY;

    range = track.maxX - track.minX;
    track.incrX = Math.pow(10, Math.floor(Math.log(range)/Math.LN10));

    tics = range / track.incrX;
    if (tics < 3) {
        track.incrX /= 2;
    } else if (tics > 7) {
        track.incrX *= 2;
    }
}

function trackToolsDisplay(canvasId, track, isMetric) {

    var canvas = document.getElementById(canvasId);
    var ctx = canvas.getContext('2d');

    track.metric = isMetric;

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
    ctx.moveTo(track.left, track.bottom);
    ctx.lineTo(track.left, track.getY(0));

    for (var i in track.points) {
        ctx.lineTo(track.getX(i), track.getY(i));
    }

    ctx.lineTo(track.right, track.bottom);
    ctx.lineTo(track.left, track.bottom);
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

    // Indicate the most difficult kilometer

    if (track.difficult.score > 0) {

        var start = track.getX(track.difficult.start);
        var end   = track.getX(track.difficult.end);

        ctx.fillStyle = 'rgba(200,160,160,0.5)';
        ctx.fillRect(start,
                     track.top,
                     end - start,
                     track.bottom - track.top);
    }
}


