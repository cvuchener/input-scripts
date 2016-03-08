const Geometry = importScript ("imports/geometry.js");

function init (map) {
	this.map = [];
	map.forEach (function (elem) {
		var region = { state: false };
		Object.assign (region, elem);
		this.map.push (region);
	}.bind (this));
}

function updatePos (x, y) {
	this.map.forEach (function (region) {
		var new_state = true;
		switch (region.type) {
		case "rect":
			if (region.hasOwnProperty ("min_x"))
				new_state &= x >= region.min_x;
			if (region.hasOwnProperty ("max_x"))
				new_state &= x <= region.max_x;
			if (region.hasOwnProperty ("min_y"))
				new_state &= y >= region.min_y;
			if (region.hasOwnProperty ("max_y"))
				new_state &= y <= region.max_y;
			break;

		case "polar":
			var r = Geometry.length ([x, y]);
			var angle = Geometry.angle ([x, y]) * 180/Math.PI;
			if (region.hasOwnProperty ("min_r"))
				new_state &= r >= region.min_r;
			if (region.hasOwnProperty ("max_x"))
				new_state &= r <= region.max_r;
			if (region.hasOwnProperty ("min_angle") &&
			    region.hasOwnProperty ("max_angle")) {
				var offset = 0;
				while (region.max_angle < angle + offset)
					offset -= 360;
				while (region.max_angle >= angle + offset + 2*Math.Pi)
					offset += 360;
				new_state &= (angle + offset >= region.min_angle) &&
					     (angle + offset <= region.max_angle);
			}
			break;

		case "circle":
			new_state = Geometry.length ([x - region.center_x, y - region.center_y]) < region.radius;
			break;
		}
		if (new_state != region.state) {
			region.event (new_state);
			region.state = new_state;
		}
	});
}

function release () {
	this.map.forEach (function (region) {
		if (region.state) {
			region.event (false);
			region.state = false;
		}
	});
}
