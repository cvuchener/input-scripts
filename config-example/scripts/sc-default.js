const SC = SteamControllerDevice;

function init () {
	input.enableKeys ();
	input.setSetting (SC.SettingTrackBall, SC.TrackBallOn);
}

function finalize () {
}

function event (ev) {
}
