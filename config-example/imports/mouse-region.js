/*
 * The "Input Scripts Virtual Tablet" device must be recognize as a tablet.
 * This can be done by adding the following udev rule.
 *
 * SUBSYSTEM=="input", ATTRS{name}=="Input Scripts Virtual Tablet", ENV{ID_INPUT}="1", ENV{ID_INPUT_JOYSTICK}="", ENV{ID_INPUT_TABLET}="1", ENV{ID_INPUT_TOUCHPAD}=""
 */

const AbsMax = 32767;
const AbsMin = -32768;

function init (screen_size, screen_region, input_region) {
	this.screen_size = screen_size;
	this.screen_region = screen_region;
	this.input_region = input_region;

	this.tablet = new UInput ();
	this.tablet.name = "Input Scripts Virtual Tablet";
	this.tablet.setKey (BTN_TOOL_FINGER);
	this.tablet.setAbs (ABS_X, AbsMin, AbsMax, 0, 0);
	this.tablet.setAbs (ABS_Y, AbsMin, AbsMax, 0, 0);
	this.tablet.create ();

	this.touching = false;
}

function finalize () {
	this.tablet.destroy ();
}

function update (pos) {
	if (!this.touching) {
		this.touching = true;
		this.tablet.sendKey (BTN_TOOL_FINGER, 1);
	}
	for (var i = 0; i < 2; ++i) {
		var c = pos[i];
		if (c < this.input_region[i])
			c = this.input_region[i];
		else if (c > this.input_region[2+i])
			c = this.input_region[2+i];
		c = (c - this.input_region[i]) / (this.input_region[2+i] - this.input_region[i]);
		c = (this.screen_region[i] * (1-c) + this.screen_region[2+i] * c) / this.screen_size[i];
		c = AbsMin * (1-c) + AbsMax * c;
		this.tablet.sendAbs (ABS_X+i, c);
	}
	this.tablet.sendSyn (SYN_REPORT);
}

function release () {
	if (this.touching) {
		this.touching = false;
		this.tablet.sendKey (BTN_TOOL_FINGER, 0);
		this.tablet.sendSyn (SYN_REPORT);
	}
}
