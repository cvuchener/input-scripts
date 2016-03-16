const Q = importScript ("imports/quaternion.js");
const G = importScript ("imports/geometry.js");

function init (callback, stepsize) {
	this.callback = callback;
	this.stepsize = stepsize;
	this.last_dir_set = false;
	this.r = [ 0, 0 ];
}

function update (w, x, y, z) {
	var q = Q.normalize ([ w, x, y, z ]);
	if (Number.isNaN (q[0]) || Number.isNaN (q[0]) || Number.isNaN (q[0]) || Number.isNaN (q[0]))
		return;
	if (this.last_dir_set) {
		var dir = Q.transform (q, this.last_dir);

		var diff = [ 180/Math.PI * G.shortestAngleDiff (G.angle ([dir[1], dir[0]]), 0),
			     180/Math.PI * G.shortestAngleDiff (G.angle ([dir[1], -dir[2]]), 0)];
		for (var i = 0; i < 2; ++i) {
			this.r[i] += diff[i];
			var steps = Math.floor (Math.abs (this.r[i]) / this.stepsize) * Math.sign (this.r[i]);
			this.r[i] -= steps * this.stepsize;
			if (steps != 0)
				this.callback (i, steps);
		}
	}
	this.last_dir_set = true;
	this.last_dir = Q.transform (Q.conjugate (q), [0, 1, 0]);
}

function release () {
	this.last_dir_set = false;
}
