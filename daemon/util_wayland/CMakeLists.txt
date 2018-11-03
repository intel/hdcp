cmake_minimum_required(VERSION 2.8.12)

project(util_wayland CXX)

find_package(OpenSSL REQUIRED)
pkg_check_modules(LIBDRM REQUIRED libdrm)

include_directories( ../ )
include_directories(${LIBDRM_INCLUDE_DIRS})
include_directories( $ENV{WLD}/include $ENV{WLD}/include/libdrm  ../../external/libdrm ../../external/libdrm/include/drm wl_base )


set (CMAKE_CXX_STANDARD 11)

add_definitions(-g -Wall)

add_library( ${PROJECT_NAME} SHARED
	display_window_util_wl.cpp
	wl_base/global_wayland.cpp
	wl_base/ias-shell-protocol.c
	wl_base/ias-backend-protocol.c
	wl_base/ivi-application-protocol.c
	wl_base/xdg-shell-unstable-v6-protocol.c
	wl_base/ias-backend-protocol.c
	)
set_target_properties(${PROJECT_NAME} PROPERTIES
                    VERSION ${HDCP_VERSION} SOVERSION ${HDCP_VERSION_MAJOR})

target_link_libraries( ${PROJECT_NAME}
	)



		
