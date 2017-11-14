const SteamControllerFF = importScript ("imports/sc-ff.js");
const SC = SteamControllerDevice;

function init () {
	input.disableKeys ();
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOff);
	input.setTouchPadEventMode (true, false);

	this.uinput = new UInput ();
	this.uinput.name = "Valve Steam Controller Gamepad";
	this.uinput.busType = 3;
	this.uinput.vendor = 0x28de;
	this.uinput.product = 0x1102;

	this.uinput.setKey (BTN_SOUTH);
	this.uinput.setKey (BTN_EAST);
	this.uinput.setKey (BTN_NORTH);
	this.uinput.setKey (BTN_WEST);
	this.uinput.setKey (BTN_C);
	this.uinput.setKey (BTN_Z);
	this.uinput.setKey (BTN_TL);
	this.uinput.setKey (BTN_TR);
	this.uinput.setKey (BTN_TL2);
	this.uinput.setKey (BTN_TR2);
	this.uinput.setKey (BTN_SELECT);
	this.uinput.setKey (BTN_START);
	this.uinput.setKey (BTN_MODE);
	this.uinput.setKey (BTN_THUMBL);
	this.uinput.setKey (BTN_THUMBR);
	this.uinput.setKey (0x13f);

	this.uinput.setAbs (ABS_X, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_Y, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_HAT0X, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_HAT0Y, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_HAT1X, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_HAT1Y, -32768, 32767, 16, 128);
	this.uinput.setAbs (ABS_HAT2X, 0, 255, 0, 0);
	this.uinput.setAbs (ABS_HAT2Y, 0, 255, 0, 0);

	this.ff = Object.create (SteamControllerFF);
	this.ff.init (this.uinput);

	this.uinput.create ();

	this.remapper = new Remapper (input, this.uinput);
	this.remapper.addEvent (EV_KEY, SC.BtnA).setCode (BTN_SOUTH);
	this.remapper.addEvent (EV_KEY, SC.BtnB).setCode (BTN_EAST);
	this.remapper.addEvent (EV_KEY, SC.BtnX).setCode (BTN_WEST);
	this.remapper.addEvent (EV_KEY, SC.BtnY).setCode (BTN_NORTH);
	this.remapper.addEvent (EV_KEY, SC.BtnShoulderLeft).setCode (BTN_TL);
	this.remapper.addEvent (EV_KEY, SC.BtnShoulderRight).setCode (BTN_TR);
	this.remapper.addEvent (EV_KEY, SC.BtnTriggerLeft).setCode (BTN_TL2);
	this.remapper.addEvent (EV_KEY, SC.BtnTriggerRight).setCode (BTN_TR2);
	this.remapper.addEvent (EV_KEY, SC.BtnGripLeft).setCode (BTN_C);
	this.remapper.addEvent (EV_KEY, SC.BtnGripRight).setCode (BTN_Z);
	this.remapper.addEvent (EV_KEY, SC.BtnSelect).setCode (BTN_SELECT);
	this.remapper.addEvent (EV_KEY, SC.BtnMode).setCode (BTN_MODE);
	this.remapper.addEvent (EV_KEY, SC.BtnStart).setCode (BTN_START);
	this.remapper.addEvent (EV_KEY, SC.BtnClickLeft)
		.addModifierMax (SC.EventBtn, SC.BtnTouchLeft, 0)
		.setCode (BTN_THUMBL);
	this.remapper.addEvent (EV_KEY, SC.BtnClickLeft)
		.addModifierMin (SC.EventBtn, SC.BtnTouchLeft, 1)
		.setCode (0x13f);
	this.remapper.addEvent (EV_KEY, SC.BtnClickRight).setCode (BTN_THUMBR);
	this.remapper.addEvent (EV_ABS, SC.AbsLeftX)
		.addModifierMax (SC.EventBtn, SC.BtnTouchLeft, 0)
		.setCode (ABS_X);
	this.remapper.addEvent (EV_ABS, SC.AbsLeftY)
		.addModifierMax (SC.EventBtn, SC.BtnTouchLeft, 0)
		.setTransform (-1, 1, 0)
		.setCode (ABS_Y);
	this.remapper.addEvent (EV_ABS, SC.AbsLeftX)
		.addModifierMin (SC.EventBtn, SC.BtnTouchLeft, 1)
		.setCode (ABS_HAT0X);
	this.remapper.addEvent (EV_ABS, SC.AbsLeftY)
		.addModifierMin (SC.EventBtn, SC.BtnTouchLeft, 1)
		.setTransform (-1, 1, 0)
		.setCode (ABS_HAT0Y);
	this.remapper.addEvent (EV_ABS, SC.AbsRightX).setCode (ABS_HAT1X);
	this.remapper.addEvent (EV_ABS, SC.AbsRightY)
		.setTransform (-1, 1, 0)
		.setCode (ABS_HAT1Y);
	this.remapper.addEvent (EV_ABS, SC.AbsLeftTrigger).setCode (ABS_HAT2Y);
	this.remapper.addEvent (EV_ABS, SC.AbsRightTrigger).setCode (ABS_HAT2X);
	this.remapper.addEvent (EV_SYN, SYN_REPORT);
	this.remapper.connect ();
}

function finalize () {
	this.uinput.destroy ();
}
