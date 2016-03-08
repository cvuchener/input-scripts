const Q = importScript ("imports/quaternion.js");
const Geometry = importScript ("imports/geometry.js");
const angle = Geometry.angle;
const length = Geometry.length;

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
	var dir = Q.transform (q, [0, 1, 0]);

	if (!this.last_dir_set) {
		this.last_dir_set = true;
		this.last_dir = dir;
		return;
	}

	var diff = [ 180/Math.PI * Geometry.shortestAngleDiff (
			angle ([ -dir[0], dir[1] ]),
			angle ([ -this.last_dir[0], this.last_dir[1] ])),
		     180/Math.PI * Geometry.shortestAngleDiff (
			angle ([ dir[2], length (dir.slice (0, 2)) ]),
			angle ([ this.last_dir[2], length (this.last_dir.slice (0, 2)) ]))
	];
	for (var i = 0; i < 2; ++i) {
		this.r[i] += diff[i];
		var steps = Math.floor (Math.abs (this.r[i]) / this.stepsize) * Math.sign (this.r[i]);
		this.r[i] -= steps * this.stepsize;
		if (steps != 0)
			this.callback (i, steps);
	}

	this.last_dir = dir;
}

function release () {
	this.last_dir_set = false;
}
