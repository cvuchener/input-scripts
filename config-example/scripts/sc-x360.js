const SteamControllerFF = importScript ("imports/sc-ff.js");
const Remapper = importScript ("imports/remapper.js");
const ButtonMap = importScript ("imports/button-map.js");
const SC = SteamControllerDevice;

function inverse (value) {
	return -value;
}

function init () {
	input.disableKeys ();
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOff);

	this.uinput = new UInput ();
	this.uinput.name = "Microsoft X-Box 360 pad";
	this.uinput.busType = 3;
	this.uinput.vendor = 0x045e;
	this.uinput.product = 0x028e;
	this.uinput.version = 0x0114;

	this.uinput.setKey (BTN_SOUTH);
	this.uinput.setKey (BTN_EAST);
	this.uinput.setKey (BTN_NORTH);
	this.uinput.setKey (BTN_WEST);
	this.uinput.setKey (BTN_TL);
	this.uinput.setKey (BTN_TR);
	this.uinput.setKey (BTN_SELECT);
	this.uinput.setKey (BTN_START);
	this.uinput.setKey (BTN_MODE);
	this.uinput.setKey (BTN_THUMBL);
	this.uinput.setKey (BTN_THUMBR);

	this.uinput.setAbs (ABS_X, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_Y, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_Z, 0, 255, 0, 0);
	this.uinput.setAbs (ABS_RX, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_RY, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_RZ, 0, 255, 0, 0);
	this.uinput.setAbs (ABS_HAT0X, -1, 1, 0, 0);
	this.uinput.setAbs (ABS_HAT0Y, -1, 1, 0, 0);

	this.ff = Object.create (SteamControllerFF);
	this.ff.init (this.uinput);

	this.uinput.create ();

	this.remapper = Object.create (Remapper);
	this.remapper.init (input, this.uinput, [
		{ type: EV_KEY, code: SC.BtnA, new_code: BTN_SOUTH },
		{ type: EV_KEY, code: SC.BtnB, new_code: BTN_EAST },
		{ type: EV_KEY, code: SC.BtnX, new_code: BTN_NORTH },
		{ type: EV_KEY, code: SC.BtnY, new_code: BTN_WEST },
		{ type: EV_KEY, code: SC.BtnShoulderLeft, new_code: BTN_TL },
		{ type: EV_KEY, code: SC.BtnShoulderRight, new_code: BTN_TR },
		{ type: EV_KEY, code: SC.BtnSelect, new_code: BTN_SELECT },
		{ type: EV_KEY, code: SC.BtnMode, new_code: BTN_MODE },
		{ type: EV_KEY, code: SC.BtnStart, new_code: BTN_START },
		{ type: EV_KEY, code: SC.BtnClickLeft,
			modifiers: [{ type: EV_KEY, code: SC.BtnTouchLeft, max: 0 }],
			new_code: BTN_THUMBL },
		{ type: EV_KEY, code: SC.BtnClickRight, new_code: BTN_THUMBR },
		{ type: EV_ABS, code: SC.AbsLeftX,
			modifiers: [{ type: EV_KEY, code: SC.BtnTouchLeft, max: 0 }],
			new_code: ABS_X },
		{ type: EV_ABS, code: SC.AbsLeftY,
			modifiers: [{ type: EV_KEY, code: SC.BtnTouchLeft, max: 0 }],
			new_code: ABS_Y, transform: this.inverse },
		{ type: EV_ABS, code: SC.AbsRightX, new_code: ABS_RX },
		{ type: EV_ABS, code: SC.AbsRightY, new_code: ABS_RY },
		{ type: EV_ABS, code: SC.AbsLeftTrigger, new_code: ABS_Z },
		{ type: EV_ABS, code: SC.AbsRightTrigger, new_code: ABS_RZ },
	]);

	this.dpad = Object.create (ButtonMap);
	this.dpad.init ([
		{ type: "polar", min_r: "8192", min_angle: -60, max_angle: 60,
			event: this.touchButton.bind (this, SC.HapticLeft, EV_ABS, ABS_HAT0X, 1)},
		{ type: "polar", min_r: "8192", min_angle: 30, max_angle: 150,
			event: this.touchButton.bind (this, SC.HapticLeft, EV_ABS, ABS_HAT0Y, -1)},
		{ type: "polar", min_r: "8192", min_angle: 120, max_angle: 240,
			event: this.touchButton.bind (this, SC.HapticLeft, EV_ABS, ABS_HAT0X, -1)},
		{ type: "polar", min_r: "8192", min_angle: -150, max_angle: -30,
			event: this.touchButton.bind (this, SC.HapticLeft, EV_ABS, ABS_HAT0Y, 1)}]);
}

function finalize () {
	this.uinput.destroy ();
}

function touchButton (actuator, type, code, value, pressed) {
	input.hapticFeedback (actuator, 0x8000, 0, 1);
	if (pressed)
		this.uinput.sendEvent (type, code, value);
	else
		this.uinput.sendEvent (type, code, 0);
}

function event (type, code, value) {
	if (type == EV_KEY && code == SC.BtnClickLeft && value == 0 &&
	    input.getValue (EV_KEY, SC.BtnTouchLeft) == 1) {
		this.dpad.release ();
	}
	switch (type) {
	case EV_KEY:
	case EV_ABS:
		this.remapper.event (type, code, value);
		break;
	
	case EV_SYN:
		if (input.getValue (EV_KEY, SC.BtnTouchLeft) == 1 &&
		    input.getValue (EV_KEY, SC.BtnClickLeft) == 1) {
			this.dpad.updatePos (input.getValue (EV_ABS, SC.AbsLeftX),
					input.getValue (EV_ABS, SC.AbsLeftY));
		}
		this.uinput.sendSyn (code);
	}
}
