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

set(HDCP_CXX_FLAGS -fPIE -fPIC -pie -z,noexecstack -z,relro -z,now -Wall -Wformat -Wformat-security -std=c++11)
set(HDCP_C_FLAGS -fPIE -fPIC -pie -z,noexecstack -z,relro -z,now -Wall -Wformat -Wformat-security)

add_definitions(-DLINUX)

if (DEFINED CMAKE_BUILD_TYPE AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(STATUS "cmake build type is DEBUG")

    set(HDCP_CXX_FLAGS ${HDCP_CXX_FLAGS} -g)
    set(HDCP_C_FLAGS ${HDCP_C_FLAGS} -g)
    add_definitions(-DLOG_CONSOLE)
    add_definitions(-DHDCP_USE_VERBOSE_LOGGING)
    add_definitions(-DHDCP_USE_FUNCTION_LOGGING)
    add_definitions(-DHDCP_USE_LINK_FUNCTION_LOGGING)
endif ()

if (DEFINED CMAKE_BUILD_TYPE AND "${CMAKE_BUILD_TYPE}" STREQUAL "ReleaseInternal")
    message(STATUS "cmake build type is \"${CMAKE_BUILD_TYPE}\"")

    add_definitions(-DLOG_CONSOLE)
    add_definitions(-DHDCP_USE_VERBOSE_LOGGING)
    add_definitions(-DHDCP_USE_FUNCTION_LOGGING)
endif ()

if (DEFINED CMAKE_BUILD_TYPE AND "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "cmake build type is \"${CMAKE_BUILD_TYPE}\"")

    set(HDCP_CXX_FLAGS ${HDCP_CXX_FLAGS} -O2 -D_FORTIFY_SOURCE=2)
    set(HDCP_C_FLAGS ${HDCP_C_FLAGS} -O2 -D_FORTIFY_SOURCE=2)
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

add_subdirectory(common ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/common)
add_subdirectory(sdk ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/sdk)
add_subdirectory(daemon ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/daemon)

set(CMAKE_INSTALL_BINDIR /usr/bin)
set(CMAKE_INSTALL_LIBDIR /usr/lib/x86_64-linux-gnu)
install (FILES ${CMAKE_BINARY_DIR}/libhdcpsdk.so DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT cp)
install (FILES ${CMAKE_BINARY_DIR}/hdcpd DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT cp)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "intel-uapi-hdcp")
set(CPACK_PACKAGE_VENDOR "Intel")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "intel-uapi-hdcp deb package for internal usage only")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Intel")
include(CPack)
