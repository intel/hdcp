# Copyright (c) 2018, Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

cmake_minimum_required(VERSION 2.8.12)

include(GNUInstallDirs)

add_definitions(-DLINUX)

# Forbid in-tree building
if(${CMAKE_SOURCE_DIR} MATCHES ${CMAKE_BINARY_DIR})
    message(STATUS "Please do out-of-tree build")
    message(FATAL_ERROR "In-tree-build detected!")
endif()

# Set compile flags
set(HDCP_COMMON_COMPILE_FLAGS
    -O2
    -D_FORTIFY_SOURCE=2
    -fstack-protector-all
    -foptimize-strlen
    -fPIE
    -fPIC
    -Wall
    -Wformat
    -Wformat-security
)

set(HDCP_C_FLAGS ${HDCP_COMMON_COMPILE_FLAGS})
set(HDCP_CXX_FLAGS ${HDCP_COMMON_COMPILE_FLAGS} -std=c++11)
set(HDCP_LD_FLAGS "-z noexecstack -z relro -z now -pie") 

# Set build type
if(DEFINED CMAKE_BUILD_TYPE AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(STATUS "cmake build type is DEBUG")

    set(HDCP_CXX_FLAGS ${HDCP_CXX_FLAGS} -g)
    set(HDCP_C_FLAGS ${HDCP_C_FLAGS} -g)
    add_definitions(-DLOG_CONSOLE)
    add_definitions(-DHDCP_USE_VERBOSE_LOGGING)
    add_definitions(-DHDCP_USE_FUNCTION_LOGGING)
    add_definitions(-DHDCP_USE_LINK_FUNCTION_LOGGING)
endif()

if(DEFINED CMAKE_BUILD_TYPE AND "${CMAKE_BUILD_TYPE}" STREQUAL "ReleaseInternal")
    message(STATUS "cmake build type is \"${CMAKE_BUILD_TYPE}\"")

    add_definitions(-DLOG_CONSOLE)
    add_definitions(-DHDCP_USE_VERBOSE_LOGGING)
    add_definitions(-DHDCP_USE_FUNCTION_LOGGING)
endif()

if(DEFINED CMAKE_BUILD_TYPE AND "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "cmake build type is \"${CMAKE_BUILD_TYPE}\"")
endif()

# Set version
set(HDCP_VERSION_MAJOR 0)
set(HDCP_VERSION_MINOR 1)
set(HDCP_VERSION_PATCH 1)
set(HDCP_VERSION
    ${HDCP_VERSION_MAJOR}.${HDCP_VERSION_MINOR}.${HDCP_VERSION_PATCH})

# Include sub project sdk daemon
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})


add_subdirectory(common ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/common)
add_subdirectory(sdk ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/sdk)
add_subdirectory(daemon/util_wayland ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/daemon/util_wayland)
add_subdirectory(daemon ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/daemon)

configure_file(${CMAKE_SOURCE_DIR}/config/libhdcpsdk.pc.in
            ${CMAKE_BINARY_DIR}/libhdcpsdk.pc
            @ONLY)

# Set install path
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr/local")
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)


install(PROGRAMS ${CMAKE_BINARY_DIR}/hdcpd
        DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT cp)
install(FILES ${CMAKE_SOURCE_DIR}/sdk/hdcpapi.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT cp)
install(FILES ${CMAKE_BINARY_DIR}/libhdcpsdk.so
        DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT cp)
install(FILES ${CMAKE_BINARY_DIR}/libhdcpsdk.so.${HDCP_VERSION_MAJOR}
        DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT cp)
install(FILES ${CMAKE_BINARY_DIR}/libhdcpsdk.so.${HDCP_VERSION}
        DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT cp)
install(FILES ${CMAKE_BINARY_DIR}/libhdcpsdk.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig COMPONENT cp)
install(FILES ${CMAKE_BINARY_DIR}/libutil_wayland.so
        DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT cp)
install(FILES ${CMAKE_BINARY_DIR}/libutil_wayland.so.${HDCP_VERSION_MAJOR}
        DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT cp)
install(FILES ${CMAKE_BINARY_DIR}/libutil_wayland.so.${HDCP_VERSION}
        DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT cp)

include(${CMAKE_SOURCE_DIR}/config/systemdservice.cmake)

message(STATUS "hdcpd install dir : ${CMAKE_INSTALL_BINDIR}")
message(STATUS "libhdcpsdk.so install dir : ${CMAKE_INSTALL_LIBDIR}")

# Set package information
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "intel-uapi-hdcp")
set(CPACK_PACKAGE_VERSION ${HDCP_VERSION})
set(CPACK_PACKAGE_VENDOR "Intel")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "intel unified hdcp deb package")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${CMAKE_SOURCE_DIR}/config/postinst)

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Intel")
include(CPack)
