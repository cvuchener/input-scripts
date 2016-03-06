function init (uinput) {
	uinput.setFF (FF_PERIODIC);
	uinput.setFF (FF_SINE);
	uinput.setFF (FF_SQUARE);
	uinput.setFF (FF_TRIANGLE);
	uinput.setFF (FF_RUMBLE);
	uinput.setFFEffectsMax (32);

	this.effects = [];
	uinput.setFFUploadEffect (this.uploadEffect.bind (this));
	uinput.setFFEraseEffect (this.eraseEffect.bind (this));
	uinput.setFFStart (this.startEffect.bind (this));
	uinput.setFFStop (this.stopEffect.bind (this));

	this.gain = 1.0;
	uinput.setFFSetGain (function (gain) { this.gain = gain / 0xFFFF; }.bind (this));
}

function uploadEffect (id, effect) {
	this.effects[id] = effect;
}

function eraseEffect (id) {
}

function startEffect (id) {
	var effect = this.effects[id];
	effect.playing = true;
	switch (effect.type) {
	case FF_PERIODIC:
		system.setTimeout (function () {
			this.playPeriodicEffect (id, 0);
		}.bind (this), effect.replay_delay);
		break;
	
	case FF_RUMBLE:
		system.setTimeout (function () {
			input.hapticFeedback (true, effect.strong_magnitude * this.gain, 10000, effect.replay_length/100);
			input.hapticFeedback (false, effect.weak_magnitude * this.gain, 10000, effect.replay_length/100);
		}.bind (this), effect.replay_delay);
		break;
	}
}

function playPeriodicEffect (id, time) {
	var effect = this.effects[id];
	if (!effect.playing || time > effect.replay_length)
		return;
	var magnitude = effect.magnitude;
	if (time < effect.attack_length) {
		var c = time/effect.attack_length;

		magnitude = effect.attack_level * c + effect.magnitude * (1-c);
	}
	if (time > effect.replay_length - effect.fade_length) {
		var c = (effect.replay_length - time) /effect.fade_length;
		magnitude = effect.magnitude * c + effect.fade_level * (1-c);
	}
	input.hapticFeedback (true, magnitude * this.gain, 0, 1);
	input.hapticFeedback (false, magnitude * this.gain, 0, 1);
	system.setTimeout (this.playPeriodicEffect.bind (this, id, time+effect.period), effect.period);
	
}

function stopEffect (id) {
	this.effects[id].playing = false;
}
