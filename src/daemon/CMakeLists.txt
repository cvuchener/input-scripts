cmake_minimum_required(VERSION 3.1)

set(INPUT_SCRIPTS_SOURCES
	Log.cpp
	JsHelpers/Thread.cpp
	Udev.cpp
	ScriptManager.cpp
	Script.cpp
	System.cpp
	classes/UInput.cpp
	classes/DBusProxy.cpp
	Driver.cpp
	InputDevice.cpp
	event/EventDriver.cpp
	event/EventDevice.cpp
	steamcontroller/SteamControllerDriver.cpp
	steamcontroller/SteamControllerReceiver.cpp
	steamcontroller/SteamControllerDevice.cpp
	Config.cpp
	DBusConnections.cpp
	main.cpp
)

_add_dbus_adaptor(INPUT_SCRIPTS_SOURCES ObjectManager)
_add_dbus_adaptor(INPUT_SCRIPTS_SOURCES Script)

add_executable(input-scripts ${INPUT_SCRIPTS_SOURCES})
target_include_directories(input-scripts PUBLIC
	${PROJECT_BINARY_DIR}/include
	${LIBEVDEV_INCLUDE_DIRS}
	${MOZJS_INCLUDE_DIRS}
	${LIBUDEV_INCLUDE_DIRS}
	${DBUSCPP_INCLUDE_DIRS}
	${JSONCPP_INCLUDE_DIRS}
)

target_link_libraries(input-scripts
	${LIBEVDEV_LIBRARIES}
	${MOZJS_LIBRARIES}
	${LIBUDEV_LIBRARIES}
	${DBUSCPP_LIBRARIES}
	${JSONCPP_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)

