const GyroMouse = importScript ("imports/gyro-mouse.js");

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
	input.setSetting (input.SettingTrackBall, input.TrackBallOff);
	input.setSetting (input.SettingOrientationSensors, input.SensorGyroQ);
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
	input.hapticFeedback (input.HapticRight, 0x40, 10000, Math.abs (d));
}

function finalize () {
	this.mouse.destroy ();
	input.setSetting (input.SettingOrientationSensors, 0);
}

function event (type, code, value) {
	switch (type) {
	case EV_KEY:
		switch (code) {
		case input.BtnTriggerLeft:
			this.mouse.sendKey (BTN_RIGHT, value);
			this.moved = true;
			break;
		case input.BtnTriggerRight:
			this.mouse.sendKey (BTN_LEFT, value);
			this.moved = true;
			break;
		}
		break;

	case EV_SYN:
		this.gyro_mouse.update (input.getValue (EV_ABS, input.AbsGyroQW),
					input.getValue (EV_ABS, input.AbsGyroQX),
					input.getValue (EV_ABS, input.AbsGyroQY),
					input.getValue (EV_ABS, input.AbsGyroQZ));
		if (this.moved) {
			this.mouse.sendSyn (code);
			this.moved = false;
		}
	}
}
