function normalize (q) {
	var l = 0;
	for (var i = 0; i < 4; ++i)
		l += q[i]*q[i];
	l = Math.sqrt (l);
	var q2 = new Array (4);
	for (var i = 0; i < 4; ++i)
		q2[i] = q[i] / l;
	return q2;
}

function conjugate (q) {
	return [ q[0], -q[1], -q[2], -q[3] ];
}

var _mul_comp = [
	[ 0, 1, 2, 3 ],
	[ 1, 0, 3, 2 ],
	[ 2, 3, 0, 1 ],
	[ 3, 2, 1, 0 ]
];
var _mul_sign = [
	[ +1, +1, +1, +1 ],
	[ +1, -1, +1, -1 ],
	[ +1, -1, -1, +1 ],
	[ +1, +1, -1, -1 ]
];

function multiply (q1, q2) {
	var q = [ 0, 0, 0, 0 ];
	for (var i = 0; i < 4; ++i) {
		for (var j = 0; j < 4; ++j) {
			q[_mul_comp[i][j]] += _mul_sign[i][j] * q1[i] * q2[j];
		}
	}
	return q;
}

function transform (q, vec) {
	//var p = [ 0, vec[0], vec[1], vec[2] ];
	//var out = multiply (multiply (q, p), conjugate (q));
	//return [ out[1], out[2], out[3] ];
	return [
		vec[0] * (q[0]*q[0] + q[1]*q[1] - q[2]*q[2] - q[3]*q[3])
			+ 2*vec[1] * (q[1]*q[2] - q[0]*q[3])
			+ 2*vec[2] * (q[0]*q[2] + q[1]*q[3]),
		vec[1] * (q[0]*q[0] - q[1]*q[1] + q[2]*q[2] - q[3]*q[3])
			+ 2*vec[2] * (q[2]*q[3] - q[0]*q[1])
			+ 2*vec[0] * (q[0]*q[3] + q[1]*q[2]),
		vec[2] * (q[0]*q[0] - q[1]*q[1] - q[2]*q[2] + q[3]*q[3])
			+ 2*vec[0] * (q[1]*q[3] - q[0]*q[2])
			+ 2*vec[1] * (q[0]*q[1] + q[2]*q[3])];
		
}
