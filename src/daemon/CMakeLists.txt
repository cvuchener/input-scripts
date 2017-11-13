cmake_minimum_required(VERSION 3.1)

set(INPUT_SCRIPTS_SOURCES
	Log.cpp
	jstpl/Thread.cpp
	jstpl/ClassManager.cpp
	Udev.cpp
	ScriptManager.cpp
	Script.cpp
	System.cpp
	classes/UInput.cpp
	classes/DBusProxy.cpp
	classes/Remapper.cpp
	classes/EventFilter.cpp
	Driver.cpp
	InputDevice.cpp
	event/EventDriver.cpp
	event/EventDevice.cpp
	Config.cpp
	DBusConnections.cpp
	main.cpp
)

_add_dbus_adaptor(INPUT_SCRIPTS_SOURCES ObjectManager)
_add_dbus_adaptor(INPUT_SCRIPTS_SOURCES Script)

if(WITH_STEAMCONTROLLER)
	add_subdirectory(steamcontroller)
endif()
if (WITH_WIIMOTE)
	add_subdirectory(wiimote)
endif()
if (WITH_HIDPP)
	add_subdirectory(hidpp)
endif()

add_executable(input-scripts ${INPUT_SCRIPTS_SOURCES})

_concat_flags(INPUT_SCRIPTS_CFLAGS
	${SIGCPP_CFLAGS}
	${LIBEVDEV_CFLAGS}
	${MOZJS_CFLAGS}
	${LIBUDEV_CFLAGS}
	${DBUSCPP_CFLAGS}
	${JSONCPP_CFLAGS}
	${XWIIMOTE_CFLAGS}
	${HIDPP_CFLAGS}
)
set_target_properties(input-scripts PROPERTIES
	COMPILE_FLAGS "${INPUT_SCRIPTS_CFLAGS}"
)

target_include_directories(input-scripts PUBLIC
	${PROJECT_BINARY_DIR}/include
)

target_link_libraries(input-scripts
	${SIGCPP_LIBRARIES}
	${LIBEVDEV_LIBRARIES}
	${MOZJS_LIBRARIES}
	${LIBUDEV_LIBRARIES}
	${DBUSCPP_LIBRARIES}
	${JSONCPP_LIBRARIES}
	${XWIIMOTE_LIBRARIES}
	${HIDPP_LIBRARIES}
	${CMAKE_THREAD_LIBS_INIT}
)

