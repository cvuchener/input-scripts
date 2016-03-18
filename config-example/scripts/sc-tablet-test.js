const MouseRegion = importScript ("imports/mouse-region.js");
const SC = SteamControllerDevice;

function init () {
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOff);

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

function event (ev) {
	switch (ev.type) {
	case SC.EventBtn:
		if (ev.code == SC.BtnTouchRight && ev.value == 0)
			this.mouse_region.release ();
		break;

	case EV_SYN:
		if (input.getEvent ({ type: SC.EventBtn, code: SC.BtnTouchRight }).value == 1) {
			this.mouse_region.update ([
				input.getEvent ({ type: SC.EventAbs, code: SC.AbsRightX }).value,
				-input.getEvent ({ type: SC.EventAbs, code: SC.AbsRightY }).value
			]);
		}
		break;
	}
}
