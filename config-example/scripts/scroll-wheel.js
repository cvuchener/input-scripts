({
"step": 15,

"init": function (event_handler) {
	this.pressed = false;
	this.event = event_handler;
},

"updatePos": function (x, y) {
	var angle = 0;
	if (x != 0)
		angle = Math.atan (y/x) + Math.PI * (1 - Math.sign (x))/2;
	else
		angle = Math.PI/2 * Math.sign (y);
	angle *= 180/Math.PI;

	if (!this.pressed) {
		this.pressed = true;
		this.last_angle = angle;
		this.remainder = 0;
	}
	else {
		var diff = angle - this.last_angle
		// Find the shortest difference between angle angle last_angle
		var sign = Math.sign (diff);
		while (Math.abs (diff) > Math.abs (diff-sign*360))
			diff -= sign*360;

		diff += this.remainder;
		var steps = Math.floor (Math.abs (diff) / this.step) * Math.sign (diff);
		if (steps != 0)
			this.event (steps);
		this.remainder = diff - steps * this.step;
		this.last_angle = angle;
	}
},

"release": function (x, y) {
	this.pressed = false;
}
})
