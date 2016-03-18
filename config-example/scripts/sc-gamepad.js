const SteamControllerFF = importScript ("imports/sc-ff.js");
const Remapper = importScript ("imports/remapper.js");
const SC = SteamControllerDevice;

function init () {
	input.disableKeys ();
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOff);

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
	this.uinput.setAbs (ABS_GAS, 0, 255, 0, 0);
	this.uinput.setAbs (ABS_BRAKE, 0, 255, 0, 0);

	this.ff = Object.create (SteamControllerFF);
	this.ff.init (this.uinput);

	this.uinput.create ();

	this.remapper = Object.create (Remapper);
	this.remapper.init (input, this.uinput, [
		{ type: EV_KEY, code: SC.BtnA, new_code: BTN_SOUTH },
		{ type: EV_KEY, code: SC.BtnB, new_code: BTN_EAST },
		{ type: EV_KEY, code: SC.BtnX, new_code: BTN_WEST },
		{ type: EV_KEY, code: SC.BtnY, new_code: BTN_NORTH },
		{ type: EV_KEY, code: SC.BtnShoulderLeft, new_code: BTN_TL },
		{ type: EV_KEY, code: SC.BtnShoulderRight, new_code: BTN_TR },
		{ type: EV_KEY, code: SC.BtnTriggerLeft, new_code: BTN_TL2 },
		{ type: EV_KEY, code: SC.BtnTriggerRight, new_code: BTN_TR2 },
		{ type: EV_KEY, code: SC.BtnGripLeft, new_code: BTN_C },
		{ type: EV_KEY, code: SC.BtnGripRight, new_code: BTN_Z },
		{ type: EV_KEY, code: SC.BtnSelect, new_code: BTN_SELECT },
		{ type: EV_KEY, code: SC.BtnMode, new_code: BTN_MODE },
		{ type: EV_KEY, code: SC.BtnStart, new_code: BTN_START },
		{ type: EV_KEY, code: SC.BtnClickLeft,
			modifiers: [{ type: SC.EventBtn, code: SC.BtnTouchLeft, max: 0 }],
			new_code: 0x13f },
		{ type: EV_KEY, code: SC.BtnClickLeft,
			modifiers: [{ type: SC.EventBtn, code: SC.BtnTouchLeft, min: 1 }],
			new_code: BTN_THUMBL },
		{ type: EV_KEY, code: SC.BtnClickRight, new_code: BTN_THUMBR },
		{ type: EV_ABS, code: SC.AbsLeftX,
			modifiers: [{ type: SC.EventBtn, code: SC.BtnTouchLeft, max: 0 }],
			new_code: ABS_X },
		{ type: EV_ABS, code: SC.AbsLeftY,
			modifiers: [{ type: SC.EventBtn, code: SC.BtnTouchLeft, max: 0 }],
			new_code: ABS_Y },
		{ type: EV_ABS, code: SC.AbsLeftX,
			modifiers: [{ type: SC.EventBtn, code: SC.BtnTouchLeft, min: 1 }],
			new_code: ABS_HAT0X },
		{ type: EV_ABS, code: SC.AbsLeftY,
			modifiers: [{ type: SC.EventBtn, code: SC.BtnTouchLeft, min: 1 }],
			new_code: ABS_HAT0Y },
		{ type: EV_ABS, code: SC.AbsRightX, new_code: ABS_HAT1X },
		{ type: EV_ABS, code: SC.AbsRightY, new_code: ABS_HAT1Y },
		{ type: EV_ABS, code: SC.AbsLeftTrigger, new_code: ABS_GAS },
		{ type: EV_ABS, code: SC.AbsRightTrigger, new_code: ABS_BRAKE },
	]);

}

function finalize () {
	this.uinput.destroy ();
}

function event (ev) {
	switch (ev.type) {
	case SC.EventBtn:
	case SC.EventAbs:
		this.remapper.event (ev.type, ev.code, ev.value);
		break;
	
	case EV_SYN:
		this.uinput.sendSyn (ev.code);
	}
}
