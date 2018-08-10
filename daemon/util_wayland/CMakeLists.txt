cmake_minimum_required( VERSION 2.6 )
project(display-daemon)

include_directories( ../ )
include_directories( $ENV{WLD}/include $ENV{WLD}/include/libdrm ../../external/libdrm ../../external/libdrm/include/drm wl_base )

set (CMAKE_CXX_STANDARD 11)

add_definitions(-g -Wall)

add_library( util_wayland SHARED
	display_window_util_wl.cpp
	wl_base/global_wayland.cpp
	wl_base/ias-shell-protocol.c
	wl_base/ias-backend-protocol.c
	wl_base/ivi-application-protocol.c
	wl_base/xdg-shell-unstable-v6-protocol.c
	wl_base/ias-backend-protocol.c
	)

target_link_libraries( util_wayland
	)



		
