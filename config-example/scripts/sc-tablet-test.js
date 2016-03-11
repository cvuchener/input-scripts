const MouseRegion = importScript ("imports/mouse-region.js");

function init () {
	input.setSetting (input.SettingTrackBall, input.TrackBallOff);

	this.mouse_region = Object.create (MouseRegion);
	this.mouse_region.init (
		[3360, 1080], // Multi-monitor screen is 3360x1080
		[1440, 0, 3360, 1080], // Region is the right monitor (1920x1080+1440+0)
		[-20000, -20000, 20000, 20000] // The touchpad is a disk, using the whole range would make it impossible to get in the corners
	);
}

function finalize () {
	this.mouse_region.finalize ();
}

function event (type, code, value) {
	switch (type) {
	case EV_KEY:
		if (code == input.BtnTouchRight && value == 0)
			this.mouse_region.release ();
		break;

	case EV_SYN:
		if (input.getValue (EV_KEY, input.BtnTouchRight) == 1) {
			this.mouse_region.update ([
				input.getValue (EV_ABS, input.AbsRightX),
				-input.getValue (EV_ABS, input.AbsRightY)
			]);
		}
		break;
	}
}
