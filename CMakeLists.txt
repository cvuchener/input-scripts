cmake_minimum_required(VERSION 3.1)
project(input-scripts)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fdiagnostics-color")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -pg -O0 -Wall -fdiagnostics-color")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVDEV REQUIRED libevdev)
pkg_check_modules(MOZJS REQUIRED mozjs-38)
pkg_check_modules(LIBUDEV REQUIRED libudev)
pkg_check_modules(DBUSCPP REQUIRED dbus-c++-1)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

link_directories(
	${LIBEVDEV_LIBRARY_DIRS}
	${MOZJS_LIBRARY_DIRS}
	${LIBUDEV_LIBRARY_DIRS}
	${DBUSCPP_LIBRARY_DIRS}
	${JSONCPP_LIBRARY_DIRS}
)

macro(_add_dbus_adaptor var name)
	set(ADAPTOR_HEADER ${PROJECT_BINARY_DIR}/include/dbus/${name}InterfaceAdaptor.h)
	set(INTERFACE_XML ${CMAKE_SOURCE_DIR}/dbus/${name}.xml)
	add_custom_command(
		OUTPUT ${ADAPTOR_HEADER}
		COMMAND mkdir -p ${PROJECT_BINARY_DIR}/include/dbus
		COMMAND dbusxx-xml2cpp ${INTERFACE_XML} --adaptor=${ADAPTOR_HEADER}
		DEPENDS ${INTERFACE_XML}
	)
	set(${var} ${${var}} ${ADAPTOR_HEADER})
endmacro(_add_dbus_adaptor)

macro(_add_dbus_proxy var name)
	set(PROXY_HEADER ${PROJECT_BINARY_DIR}/include/dbus/${name}InterfaceProxy.h)
	set(INTERFACE_XML ${CMAKE_SOURCE_DIR}/dbus/${name}.xml)
	add_custom_command(
		OUTPUT ${PROXY_HEADER}
		COMMAND mkdir -p ${PROJECT_BINARY_DIR}/include/dbus
		COMMAND dbusxx-xml2cpp ${INTERFACE_XML} --proxy=${PROXY_HEADER}
		DEPENDS ${INTERFACE_XML}
	)
	set(${var} ${${var}} ${PROXY_HEADER})
endmacro(_add_dbus_proxy)

add_subdirectory(src/daemon)
add_subdirectory(src/remote)

