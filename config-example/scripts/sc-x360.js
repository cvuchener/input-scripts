const SteamControllerFF = importScript ("imports/sc-ff.js");
const ButtonMap = importScript ("imports/button-map.js");
const SC = SteamControllerDevice;

function init () {
	input.disableKeys ();
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOff);
	input.setTouchPadEventMode (true, true);

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

	this.remapper = new Remapper (input, this.uinput);
	this.remapper.addEvent (EV_KEY, SC.BtnA).setCode (BTN_SOUTH);
	this.remapper.addEvent (EV_KEY, SC.BtnB).setCode (BTN_EAST);
	this.remapper.addEvent (EV_KEY, SC.BtnX).setCode (BTN_NORTH);
	this.remapper.addEvent (EV_KEY, SC.BtnY).setCode (BTN_WEST);
	this.remapper.addEvent (EV_KEY, SC.BtnShoulderLeft).setCode (BTN_TL);
	this.remapper.addEvent (EV_KEY, SC.BtnShoulderRight).setCode (BTN_TR);
	this.remapper.addEvent (EV_KEY, SC.BtnSelect).setCode (BTN_SELECT);
	this.remapper.addEvent (EV_KEY, SC.BtnMode).setCode (BTN_MODE);
	this.remapper.addEvent (EV_KEY, SC.BtnStart).setCode (BTN_START);
	this.remapper.addEvent (EV_KEY, SC.BtnClickLeft)
		.addModifierMax (SC.EventBtn, SC.BtnTouchLeft, 0)
		.setCode (BTN_THUMBL);
	this.remapper.addEvent (EV_KEY, SC.BtnClickRight).setCode (BTN_THUMBR);
	this.remapper.addEvent (EV_ABS, SC.AbsLeftX)
		.addModifierMax (SC.EventBtn, SC.BtnTouchLeft, 0)
		.setCode (ABS_X);
	this.remapper.addEvent (EV_ABS, SC.AbsLeftY)
		.addModifierMax (SC.EventBtn, SC.BtnTouchLeft, 0)
		.setCode (ABS_Y)
		.setTransform (-1, 1, 0);
	this.remapper.addEvent (EV_ABS, SC.AbsRightX).setCode (ABS_RX);
	this.remapper.addEvent (EV_ABS, SC.AbsRightY).setCode (ABS_RY);
	this.remapper.addEvent (EV_ABS, SC.AbsLeftTrigger).setCode (ABS_Z);
	this.remapper.addEvent (EV_ABS, SC.AbsRightTrigger).setCode (ABS_RZ);
	this.remapper.addEvent (EV_SYN, SYN_REPORT);

	this.remapper.connect ();

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

	this.evfilter = new EventFilter (input);
	this.evfilter.addMatchCode (SC.EventBtn, SC.BtnClickLeft);
	this.evfilter.addMatchCode (SC.EventTouchPad, SC.TouchPadLeft);
	connect (this.evfilter, 'event', this.event.bind (this));
	this.evfilter.connect ();
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

function event (ev) {
	switch (ev.type) {
	case SC.EventBtn:
		if (ev.code == SC.BtnClickLeft && ev.value == 0 && input.keyPressed (SC.BtnTouchLeft))
			this.dpad.release ();
		break;

	case SC.EventTouchPad:
		if (ev.code == SC.TouchPadLeft && input.keyPressed (SC.BtnTouchLeft) && input.keyPressed (SC.BtnClickLeft))
			this.dpad.updatePos (ev.x, ev.y);
		break;
	}
}
