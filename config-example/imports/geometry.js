function angle (v) {
	var a;
	if (v[0] != 0)
		a = Math.atan (v[1]/v[0]) + Math.PI * (1 - Math.sign (v[0]))/2;
	else
		a = Math.PI/2 * Math.sign (v[1]);
	return a;
}

function length (v) {
	var l2 = 0;
	for (var i = 0; i < v.length; ++i)
		l2 += v[i]*v[i];
	return Math.sqrt (l2);
}

function shortestAngleDiff (a1, a2) {
	var d = a1 - a2;
	var s = Math.sign (d);
	while (Math.abs (d) > Math.abs (d - s*2*Math.PI))
		d -= s*2*Math.PI;
	return d;
}

