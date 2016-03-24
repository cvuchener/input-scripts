option(WITH_STEAMCONTROLLER "Use Steam Controller driver" OFF)

if (WITH_STEAMCONTROLLER)
	set (INPUT_SCRIPTS_SOURCES ${INPUT_SCRIPTS_SOURCES}
		steamcontroller/SteamControllerDriver.cpp
		steamcontroller/SteamControllerReceiver.cpp
		steamcontroller/SteamControllerDevice.cpp
		PARENT_SCOPE
	)
endif ()
