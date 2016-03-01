({
"init": function (input, uinput, mapping) {
	this.input = input;
	this.uinput = uinput;
	this.mapping = {};
	this.modifiers = {};
	mapping.forEach (function (ev) {
		if (this.mapping[ev.type] === undefined)
			this.mapping[ev.type] = {};
		if (this.mapping[ev.type][ev.code] === undefined)
			this.mapping[ev.type][ev.code] = [];
		var new_ev = {};
		Object.assign (new_ev, ev);
		if (!ev.hasOwnProperty ("new_type"))
			new_ev.new_type = ev.type;
		if (ev.hasOwnProperty ("modifiers")) {
			ev.modifiers.forEach (function (mod) {
				if (this.modifiers[mod.type] === undefined)
					this.modifiers[mod.type] = {};
				if (this.modifiers[mod.type][mod.code] === undefined)
					this.modifiers[mod.type][mod.code] = [];
				this.modifiers[mod.type][mod.code].push (new_ev);
			}.bind (this));
		}
		new_ev.state = this.testEvent (new_ev);
		this.mapping[ev.type][ev.code].push (new_ev);
	}.bind (this));
},

"processEvent": function (event, value) {
	var new_value = value;
	if (event.hasOwnProperty ("transform"))
		new_value = event.transform (value);
	this.uinput.sendEvent (event.new_type, event.new_code, new_value);
},

"testEvent": function (event) {
	if (event.hasOwnProperty ("modifiers")) {
		for (var i = 0; i < event.modifiers.length; ++i) {
			var mod = event.modifiers[i];
			var mod_value = this.input.getValue (mod.type, mod.code);
			if (mod.hasOwnProperty ("min") && mod_value < mod.min)
				return false;
			if (mod.hasOwnProperty ("max") && mod_value > mod.max)
				return false;
		}
	}
	return true;
},

"event": function (type, code, value) {
	if (this.mapping.hasOwnProperty (type)) {
		if (this.mapping[type].hasOwnProperty (code)) {
			this.mapping[type][code].forEach (function (ev) {
				if (this.testEvent (ev)) {
					this.processEvent (ev, value);
				}
			}.bind (this));
		}
	}
	if (this.modifiers.hasOwnProperty (type)) {
		if (this.modifiers[type].hasOwnProperty (code)) {
			this.modifiers[type][code].forEach (function (ev) {
				if (ev.state && !this.testEvent (ev)) {
					this.processEvent (ev, 0);
				}
			}.bind (this));
		}
	}
	if (this.modifiers.hasOwnProperty (type)) {
		if (this.modifiers[type].hasOwnProperty (code)) {
			this.modifiers[type][code].forEach (function (ev) {
				if (!ev.state && this.testEvent (ev)) {
					this.processEvent (ev, this.input.getValue (ev.type, ev.code));
				}
				ev.state = this.testEvent (ev);
			}.bind (this));
		}
	}
}
})
