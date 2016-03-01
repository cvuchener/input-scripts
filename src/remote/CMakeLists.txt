cmake_minimum_required(VERSION 3.1)

set(INPUT_SCRIPTS_REMOTE_SOURCES
	ObjectManager.cpp
	Script.cpp
	main.cpp
)

_add_dbus_proxy(INPUT_SCRIPTS_REMOTE_SOURCES ObjectManager)
_add_dbus_proxy(INPUT_SCRIPTS_REMOTE_SOURCES Script)

add_executable(input-scripts-remote ${INPUT_SCRIPTS_REMOTE_SOURCES})
target_include_directories(input-scripts-remote PUBLIC
	${PROJECT_BINARY_DIR}/include
	${DBUSCPP_INCLUDE_DIRS}
)

target_link_libraries(input-scripts-remote
	${DBUSCPP_LIBRARIES}
)

