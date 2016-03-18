const Remapper = importScript ("imports/remapper.js");
const ButtonMap = importScript ("imports/button-map.js");
const ScrollWheel = importScript ("imports/scroll-wheel.js");
const SC = SteamControllerDevice;

function init () {
	input.disableKeys ();
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOn);

	this.uinput = new UInput ();
	this.uinput.name = "Valve Steam Controller Desktop Mode";

	this.uinput.setKey (BTN_LEFT);
	this.uinput.setKey (BTN_RIGHT);
	this.uinput.setKey (BTN_MIDDLE);
	this.uinput.setKey (BTN_SIDE);
	this.uinput.setKey (BTN_EXTRA);
	this.uinput.setKey (KEY_LEFTCTRL);
	this.uinput.setKey (KEY_LEFTALT);
	this.uinput.setKey (KEY_LEFTSHIFT);
	this.uinput.setKey (KEY_LEFTMETA);
	this.uinput.setKey (KEY_TAB);
	this.uinput.setKey (KEY_ESC);
	this.uinput.setKey (KEY_PAGEUP);
	this.uinput.setKey (KEY_PAGEDOWN);
	this.uinput.setKey (KEY_SPACE);
	this.uinput.setKey (KEY_ENTER);
	this.uinput.setKey (KEY_UP);
	this.uinput.setKey (KEY_LEFT);
	this.uinput.setKey (KEY_RIGHT);
	this.uinput.setKey (KEY_DOWN);

	this.uinput.setRel (REL_WHEEL);

	this.uinput.create ();

	this.remapper = Object.create (Remapper);
	this.remapper.init (input, this.uinput, [
		{ type: EV_KEY, code: SC.BtnA, new_code: KEY_ENTER },
		{ type: EV_KEY, code: SC.BtnB, new_code: KEY_SPACE },
		{ type: EV_KEY, code: SC.BtnX, new_code: KEY_PAGEUP },
		{ type: EV_KEY, code: SC.BtnY, new_code: KEY_PAGEDOWN },
		{ type: EV_KEY, code: SC.BtnShoulderLeft, new_code: KEY_LEFTCTRL },
		{ type: EV_KEY, code: SC.BtnShoulderRight, new_code: KEY_LEFTALT },
		{ type: EV_KEY, code: SC.BtnTriggerLeft, new_code: BTN_RIGHT },
		{ type: EV_KEY, code: SC.BtnTriggerRight, new_code: BTN_LEFT },
		{ type: EV_KEY, code: SC.BtnGripLeft, new_code: BTN_SIDE },
		{ type: EV_KEY, code: SC.BtnGripRight, new_code: BTN_EXTRA },
		{ type: EV_KEY, code: SC.BtnSelect, new_code: KEY_TAB },
		{ type: EV_KEY, code: SC.BtnMode, new_code: KEY_LEFTMETA },
		{ type: EV_KEY, code: SC.BtnStart, new_code: KEY_ESC },
		{ type: EV_KEY, code: SC.BtnClickLeft,
			modifiers: [{ type: SC.EventBtn, code: SC.BtnTouchLeft, min: 1 }],
			new_code: BTN_MIDDLE },
		{ type: EV_KEY, code: SC.BtnClickRight, new_code: KEY_LEFTSHIFT },
	]);

	this.dpad = Object.create (ButtonMap);
	this.dpad.init ([
		{ type: "polar", min_r: "8192", min_angle: -60, max_angle: 60,
			event: this.touchButton.bind (this, SC.HapticLeft, EV_KEY, KEY_RIGHT, 1)},
		{ type: "polar", min_r: "8192", min_angle: 30, max_angle: 150,
			event: this.touchButton.bind (this, SC.HapticLeft, EV_KEY, KEY_UP, 1)},
		{ type: "polar", min_r: "8192", min_angle: 120, max_angle: 240,
			event: this.touchButton.bind (this, SC.HapticLeft, EV_KEY, KEY_LEFT, 1)},
		{ type: "polar", min_r: "8192", min_angle: -150, max_angle: -30,
			event: this.touchButton.bind (this, SC.HapticLeft, EV_KEY, KEY_DOWN, 1)}]);
	this.scroll_wheel = Object.create (ScrollWheel);
	this.scroll_wheel.init (this.scrollWheelEvent.bind (this));

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

function scrollWheelEvent (steps) {
	input.hapticFeedback (SC.HapticLeft, 0x800, 10000, Math.abs (steps));
	this.uinput.sendEvent (EV_REL, REL_WHEEL, steps);
}

function event (ev) {
	switch (ev.type) {
	case SC.EventBtn:
		if (ev.code == SC.BtnTouchLeft) {
			if (ev.value == 0)
				this.scroll_wheel.release ();
			else
				this.dpad.release ();
		}
		this.remapper.event (ev.type, ev.code, ev.value);
		break;

	case EV_SYN:
		if (input.getEvent ({ type: SC.EventBtn, code: SC.BtnTouchLeft }).value == 0) {
			this.dpad.updatePos (
				input.getEvent ({ type: SC.EventAbs, code: SC.AbsLeftX }).value,
				input.getEvent ({ type: SC.EventAbs, code: SC.AbsLeftY }).value);
		}
		else {
			this.scroll_wheel.updatePos (
				input.getEvent ({ type: SC.EventAbs, code: SC.AbsLeftX }).value,
				input.getEvent ({ type: SC.EventAbs, code: SC.AbsLeftY }).value);
		}
		this.uinput.sendSyn (ev.code);
	}
}
