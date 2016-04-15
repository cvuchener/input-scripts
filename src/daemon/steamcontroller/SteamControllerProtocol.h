#ifndef STEAM_CONTROLLER_PROTOCOL_H
#define STEAM_CONTROLLER_PROTOCOL_H

namespace SteamController {

enum RequestID: uint8_t {
	RequestDisableKeys = 0x81,
	RequestEnableKeys = 0x85,
	RequestConfigure = 0x87,
	RequestEnableMouse = 0x8e,
	RequestHapticFeedback = 0x8f,
	RequestReset = 0x95,
	RequestShutdown = 0x9f,
	RequestCalibrateTouchPads = 0xa7,
	RequestGetSerial = 0xae,
	RequestGetConnectionStatus = 0xb4,
	RequestCalibrateSensors = 0xb5,
	RequestPlaySound = 0xb6,
	RequestCalibrateJoystick = 0xbf,
	RequestSetSounds = 0xc1,
};

enum ReportType: uint8_t {
	ReportInput = 1,
	ReportConnection = 3,
};

enum ConnectionEvent: uint8_t {
	Disconnected = 1,
	Connected = 2,
	Paired = 3,
};

enum SerialType: uint8_t {
	ControllerSerial = 1,
};

}

#endif
