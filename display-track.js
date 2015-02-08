/*
  Display track data in an HTML canvas.

  This uses the JS output from track tools to create an object called
  TrackData, which displays a simple interactive graph showing an elevation
  profile, climbs, and the most difficult kilometer of the ride.

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
  that 'end' is inclusive; it's part of the climb.

  "difficult" indicates the most difficult kilometer of the ride, again
  expressed as indexes. The score is an arbitrary number, but scores <= 0
  mean there was no climb detected.

  html-example.html shows how to use this.
*/

function TrackData(inPoints, inClimbs, inDifficult, inMetric) {
    this.points    = inPoints;
    this.climbs    = inClimbs;
    this.difficult = inDifficult;
    this.metric    = inMetric;

    // These are in terms of displayed units (km or miles, meters or feet),
    // so that the increments make sense.
    this.minX  = 0;
    this.maxX  = 0;
    this.minY  = 0;
    this.maxY  = 0;
    this.incrX = 0;
    this.incrY = 0;

    // These are pixel dimensions of the graph. left, right, top and bottom
    // refer to the pixel dimensions of the graph itself (i.e. without the
    // label borders). height and width are the full image dimensions.
    this.left   = 0;
    this.right  = 0;
    this.top    = 0;
    this.bottom = 0;
    this.height = 0;
    this.width  = 0;

    this.showing_hint = false;
}

// Return the value from the underlying track, in presentation form.
TrackData.prototype.elevation = function(val) {
    return this.metric ? val : val * 3.2808;
}
    
TrackData.prototype.distance = function(val) {
    return this.metric ? (val / 1000) : ((val / 1000) * 0.62137);
}

// Return text labels, metric or not.
TrackData.prototype.elevation_label = function() {
    return this.metric ? "meters" : "feet";
}

TrackData.prototype.distance_label = function() {
    return this.metric ? "km" : "miles";
}

// Calculate the X or Y pixel position of the given data value in
// presentation form (ie metric or not).
TrackData.prototype.getRawY = function(val) {
    return this.bottom - (val - this.minY) / (this.maxY - this.minY) *
	(this.bottom - this.top);
}

TrackData.prototype.getRawX = function(val) {
    return (val - this.minX) / (this.maxX - this.minX) *
	(this.right - this.left) + this.left;
}

// Calculate the X or Y pixel position of the given point within the
// 'points' array.
TrackData.prototype.getY = function(idx) {
    return this.getRawY(this.elevation(this.points[idx][1]));
}

TrackData.prototype.getX = function(idx) {
    return this.getRawX(this.distance(this.points[idx][0]));
}

// Calculate the actual min/max of the data values, adjusting the elevations
// to round numbers, and calculate increments for tic marks.
TrackData.prototype.calculateDataRange = function() {
    // First find the actual min/max data values
    this.minX = this.maxX = this.points[0][0];
    this.minY = this.maxY = this.points[0][1];

    for (var i in this.points) {
        var x = this.points[i][0];
        var y = this.points[i][1];
        if (x > this.maxX) this.maxX = x;
        if (x < this.minX) this.minX = x;
        if (y > this.maxY) this.maxY = y;
        if (y < this.minY) this.minY = y;
    }

    // These numbers need to be in terms of the displayed values, to make
    // the tics reasonable.
    this.minX = this.distance(this.minX);
    this.maxX = this.distance(this.maxX);
    this.minY = this.elevation(this.minY);
    this.maxY = this.elevation(this.maxY);

    // Now figure out the appropriate tic increments
    var range = this.maxY - this.minY;
    if (range == 0) {
        this.incrY = 1;
    } else {
	// Increments are powers of 10, unless that produces too many or
	// too few tics.
        this.incrY = Math.pow(10, Math.floor(Math.log(range)/Math.LN10));
        var tics = range / this.incrY;
        if (tics < 3) {
            this.incrY /= 2;
        } else if (tics > 7) {
            this.incrY *= 2;
        }
    }

    // We'll adjust the Y min/max to neatly fit the increments. We won't
    // do this with the X values. That's just a display preference; the
    // Y values look better with buffer, and X is better stretched end to end.

    this.minY = Math.floor(this.minY / this.incrY) * this.incrY;
    this.maxY = Math.ceil(this.maxY / this.incrY) * this.incrY;

    range = this.maxX - this.minX;
    if (range == 0) {
        this.incrX = 1;
        this.minX -= 1;
        this.maxX += 1;
    } else {
        this.incrX = Math.pow(10, Math.floor(Math.log(range)/Math.LN10));
        tics = range / this.incrX;
        if (tics < 3) {
            this.incrX /= 2;
        } else if (tics > 7) {
            this.incrX *= 2;
        }
    }
}

// Calculate the displayed range, create the initial paths, and draw the
// graph to the given context.
TrackData.prototype.InitializeGraph = function(context) {
    this.calculateDataRange();

    this.context = context;
    this.width = context.canvas.width;
    this.height = context.canvas.height;

    this.left   = 10 + this.getLabelWidthY(); // make room for labels
    this.right  = this.width - 10;
    this.top    = 10;
    this.bottom = this.height - 30; // allow room for labels

    // Draw the elevation background gradient
    this.gradient = context.createLinearGradient(0, 0, 0, this.bottom);
    this.gradient.addColorStop("0", 'rgba(236,255,236,0.6)');
    this.gradient.addColorStop("1.0", 'rgba(32,72,32,1)');

    var path = new Path2D();
    path.moveTo(this.getX(0), this.bottom);
    path.lineTo(this.getX(0), this.getY(0));

    for (var i in this.points) {
        path.lineTo(this.getX(i), this.getY(i));
    }

    path.lineTo(this.getX(this.points.length-1), this.bottom);
    path.lineTo(this.getX(0), this.bottom);

    this.elevation_fill_path = path;

    // Create the elevation path
    this.elevation_path = new Path2D();
    this.elevation_path.moveTo(this.getX(0), this.getY(0));
    for (var i in this.points) {
	this.elevation_path.lineTo(this.getX(i), this.getY(i));
    }

    // Highlight the climbs
    this.climb_paths = new Array();
    if (this.climbs && (this.climbs.length > 0)) {
        for (var i in this.climbs) {
	    var path = new Path2D();
	    var start = this.climbs[i].start;
	    var end = this.climbs[i].end;

	    path.moveTo(this.getX(start), this.getY(start));
	    for (var p = start + 1; p <= end; p++) {
                path.lineTo(this.getX(p), this.getY(p));
	    }
	    this.climb_paths.push(path);
        }
    }

    // Indicate the most difficult kilometer
    this.most_difficult_path = new Path2D();
    if (this.difficult && this.difficult.score > 0) {
        var start = this.getX(this.difficult.start);
        var end   = this.getX(this.difficult.end);

        this.most_difficult_path.rect(start,
				      this.top,
				      end - start,
				      this.bottom - this.top);
    }
    this.render();
    context.canvas.addEventListener('mousemove', this);
}

// Handle a mouse event by showing a hint box with the appropriate
// elevation and distance. Note that we just overwrite any existing box,
// which can leave artifacts.
TrackData.prototype.handleEvent = function(event) {
    if (!event instanceof MouseEvent) return;

    var mouseX = event.pageX - this.context.canvas.offsetLeft;
    var mouseY = event.pageY - this.context.canvas.offsetTop;

    // If the mouse isn't somewhere in the elevation graph, then
    // render the base graph (without the hint box) and leave.
    if (!this.context.isPointInPath(this.elevation_fill_path,
				    mouseX, mouseY)) {
	if (this.showing_hint) this.render();
	return;
    }

    // Find the point in 'track' corresponding to mouseX.
    // TODO: binary chop?
    var idx = 0;
    for (var i in this.points) {
	idx = i;
	if (this.getX(i) >= mouseX) {
	    break;
	}
    }
    var point = this.points[idx];

    var label = this.distance(point[0]).toFixed(2) +
                " " + this.distance_label() + ", elevation " +
                this.elevation(point[1]).toFixed(0) +
                " " + this.elevation_label();

    this.context.font = '10pt sans-serif';
    var textsize = this.context.measureText(label);
    var buffer = 20;
    var boxpath = new Path2D();
    boxpath.rect(this.width / 2 - textsize.width/2 - buffer,
		 this.height - 60,
		 textsize.width + buffer * 2, 20);

    this.context.fillStyle = '#f0ffff';
    this.context.fill(boxpath);

    this.context.strokeStyle = '#306030';
    this.context.lineWidth = 2;
    this.context.stroke(boxpath);

    this.context.fillStyle = '#306030';
    this.context.fillText(label, this.width/2 - textsize.width/2,
				 this.height - 45);
    this.showing_hint = true;
}

// Get the maximum width of all the labels on the Y axis.
TrackData.prototype.getLabelWidthY = function() {
    this.context.font = '10pt sans-serif';
    var width = 0;
    for (var i = this.minY; i <= this.maxY; i += this.incrY) {
        var w = this.context.measureText(i + "").width;
        if (w > width) width = w;
    }

    return width;
}

TrackData.prototype.displayTicsY = function() {
    var ctx = this.context;
    var width = this.getLabelWidthY();
    ctx.fillStyle = '#404040';
    ctx.font = '10pt sans-serif';
    for (var i = this.minY; i <= this.maxY; i += this.incrY) {
        var w = ctx.measureText(i + "").width;
        var y = Math.round(this.getRawY(i)) + 0.5;

        ctx.fillText(i + "", (width - w) + 5, y + 4);

        ctx.beginPath();
        ctx.lineWidth = 1;
        ctx.strokeStyle = '#c0c0c0';
        ctx.moveTo(this.left, y);
        ctx.lineTo(this.right, y);
        ctx.stroke();
    }
}

TrackData.prototype.displayTicsX = function() {
    var ctx = this.context;
    ctx.fillStyle = '#404040';
    ctx.font = '10pt sans-serif';
    for (var i = this.minX + this.incrX; i <= this.maxX; i += this.incrX) {
        var v = Math.round(i - this.minX);
        var w = ctx.measureText(v + "").width;
        var x = this.getRawX(i);
        ctx.fillText(v + "", x - w/2, this.bottom + 12);
    }

    var label = "distance (" + this.distance_label() + ")";
    var w = ctx.measureText(label).width;
    ctx.fillText(label, this.width / 2 - w/2, this.bottom + 24);
}

TrackData.prototype.render = function() {
    var context = this.context;  // shorthand
    // Draw the whole background
    context.fillStyle = "#ffffff";
    context.fillRect(0, 0, context.canvas.width, context.canvas.height);
    // Draw a background for just the graph portion
    context.fillStyle = "#f0ffff";
    context.fillRect(this.left, this.top,
		     this.right - this.left, this.bottom - this.top);

    // Draw tics and labels
    this.displayTicsY();
    this.displayTicsX();

    // Draw the elevation background gradient
    context.fillStyle = this.gradient;
    context.fill(this.elevation_fill_path);

    // Draw the elevation
    context.strokeStyle = "#306030";
    context.lineWidth = 4;
    context.lineJoin = 'round';
    context.stroke(this.elevation_path);

    // Draw the climbs
    context.strokeStyle = '#a05050';
    context.lineWidth = 6;
    context.lineJoin = 'round';
    for (var i in this.climb_paths) {
	context.stroke(this.climb_paths[i]);
    }

    // Draw the most difficult km
    context.fillStyle = 'rgba(200,160,160,0.5)';
    context.fill(this.most_difficult_path);

    this.showing_hint = false;
}

function trackToolsDisplay(canvasId, track) {
    var canvas = document.getElementById(canvasId);
    var ctx = canvas.getContext('2d');

    if (track.points.length == 0) return;
    track.InitializeGraph(ctx);
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
    xmlhttp.open("GET", url);
    xmlhttp.responseType = "document";
    xmlhttp.onload = function() {
        displayTrackDataGPX(canvasId, this.response, metric);
    }
    xmlhttp.send();
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
    req.open("GET", url);
    req.onload = function() {
        var obj = JSON.parse(this.responseText);
        displayTrackDataJSON(canvasId, obj, metric);
    }

    req.send();
}


