const MouseRegion = importScript ("imports/mouse-region.js");
const SC = SteamControllerDevice;

function init () {
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOff);
	input.setTouchPadEventMode (false, true);

	this.mouse_region = Object.create (MouseRegion);
	this.mouse_region.init (
		[3360, 1080], // Multi-monitor screen is 3360x1080
		[1440, 0, 3360, 1080], // Region is the right monitor (1920x1080+1440+0)
		[-20000, -20000, 20000, 20000] // The touchpad is a disk, using the whole range would make it impossible to get in the corners
	);

	connect (input, 'event', this.event.bind (this));
}

function finalize () {
	this.mouse_region.finalize ();
}

function event (ev) {
	switch (ev.type) {
	case SC.EventBtn:
		if (ev.code == SC.BtnTouchRight && ev.value == 0)
			this.mouse_region.release ();
		break;

	case SC.EventTouchPad:
		if (ev.code == SC.TouchPadRight && input.keyPressed (SC.BtnTouchRight))
			this.mouse_region.update ([ ev.x, -ev.y ]);
		break;

	case EV_SYN:
		break;
	}
}
