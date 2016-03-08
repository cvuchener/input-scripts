const Geometry = importScript ("imports/geometry.js");

var step = 30;

function init (event_handler) {
	this.pressed = false;
	this.event = event_handler;
}

function updatePos (x, y) {
	var angle = Geometry.angle ([x, y]);

	if (!this.pressed) {
		this.pressed = true;
		this.last_angle = angle;
		this.remainder = 0;
	}
	else {
		var diff = Geometry.shortestAngleDiff (angle, this.last_angle);
		diff *= 180/Math.PI;

		diff += this.remainder;
		var steps = Math.floor (Math.abs (diff) / this.step) * Math.sign (diff);
		if (steps != 0)
			this.event (steps);
		this.remainder = diff - steps * this.step;
		this.last_angle = angle;
	}
}

function release (x, y) {
	this.pressed = false;
}
