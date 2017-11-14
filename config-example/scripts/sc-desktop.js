const ButtonMap = importScript ("imports/button-map.js");
const ScrollWheel = importScript ("imports/scroll-wheel.js");
const SC = SteamControllerDevice;

function init () {
	input.disableKeys ();
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOn);
	input.setTouchPadEventMode (false, true);

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

	this.remapper = new Remapper (input, this.uinput);
	this.remapper.addEvent (EV_KEY, SC.BtnA).setCode (KEY_ENTER);
	this.remapper.addEvent (EV_KEY, SC.BtnB).setCode (KEY_SPACE);
	this.remapper.addEvent (EV_KEY, SC.BtnX).setCode (KEY_PAGEUP);
	this.remapper.addEvent (EV_KEY, SC.BtnY).setCode (KEY_PAGEDOWN);
	this.remapper.addEvent (EV_KEY, SC.BtnShoulderLeft).setCode (KEY_LEFTCTRL);
	this.remapper.addEvent (EV_KEY, SC.BtnShoulderRight).setCode (KEY_LEFTALT);
	this.remapper.addEvent (EV_KEY, SC.BtnTriggerLeft).setCode (BTN_RIGHT);
	this.remapper.addEvent (EV_KEY, SC.BtnTriggerRight).setCode (BTN_LEFT);
	this.remapper.addEvent (EV_KEY, SC.BtnGripLeft).setCode (BTN_SIDE);
	this.remapper.addEvent (EV_KEY, SC.BtnGripRight).setCode (BTN_EXTRA);
	this.remapper.addEvent (EV_KEY, SC.BtnSelect).setCode (KEY_TAB);
	this.remapper.addEvent (EV_KEY, SC.BtnMode).setCode (KEY_LEFTMETA);
	this.remapper.addEvent (EV_KEY, SC.BtnStart).setCode (KEY_ESC);
	this.remapper.addEvent (EV_KEY, SC.BtnClickLeft)
		.addModifierMin (SC.EventBtn, SC.BtnTouchLeft, 1)
		.setCode (BTN_MIDDLE);
	this.remapper.addEvent (EV_KEY, SC.BtnClickRight).setCode (KEY_LEFTSHIFT);
	this.remapper.addEvent (EV_SYN, SYN_REPORT);
	this.remapper.connect ();

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

	this.evfilter = new EventFilter (input);
	this.evfilter.addMatchCode (SC.EventBtn, SC.BtnTouchLeft);
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
		break;

	case SC.EventTouchPad:
		if (ev.code == SC.TouchPadLeft) {
			if (input.keyPressed (SC.BtnTouchLeft)) {
				if (ev.x != 0 || ev.y != 0)
					this.scroll_wheel.updatePos (ev.x, ev.y);
			}
			else
				this.dpad.updatePos (ev.x, ev.y);
		}
		break;
	}
}
