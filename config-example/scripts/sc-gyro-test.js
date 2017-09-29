const GyroMouse = importScript ("imports/gyro-mouse.js");
const SC = SteamControllerDevice;

function init () {
	this.mouse = new UInput ();
	this.mouse.name = "Gyro mouse test";
	this.mouse.setKey (BTN_LEFT);
	this.mouse.setKey (BTN_RIGHT);
	this.mouse.setRel (REL_X);
	this.mouse.setRel (REL_Y);
	this.mouse.create ();

	this.gyro_mouse = Object.create (GyroMouse);
	this.gyro_mouse.init (this.mouseMove.bind (this), 0.05);

	input.disableKeys ();
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOff);
	input.setSetting (SC.SettingOrientationSensors, SC.OrientationQuaternion);

	connect (input, 'event', this.event.bind (this));
}

function mouseMove (axis, d) {
	this.moved = true;
	switch (axis) {
	case 0:
		this.mouse.sendRel (REL_X, d);
		break;
	case 1:
		this.mouse.sendRel (REL_Y, d);
		break;
	}
	input.hapticFeedback (SC.HapticRight, 0x40, 10000, Math.abs (d));
}

function finalize () {
	this.mouse.destroy ();
	input.setSetting (SC.SettingOrientationSensors, 0);
}

function event (ev) {
	switch (ev.type) {
	case SC.EventBtn:
		switch (ev.code) {
		case SC.BtnTriggerLeft:
			this.mouse.sendKey (BTN_RIGHT, ev.value);
			this.moved = true;
			break;
		case SC.BtnTriggerRight:
			this.mouse.sendKey (BTN_LEFT, ev.value);
			this.moved = true;
			break;
		}
		break;

	case SC.EventOrientation:
		this.gyro_mouse.update (ev.w, ev.x, ev.y, ev.z);
		break;

	case EV_SYN:
		if (this.moved) {
			this.mouse.sendSyn (ev.code);
			this.moved = false;
		}
	}
}
