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

project(hdcpsdk CXX)

find_package(Threads REQUIRED)

get_filename_component(HDCP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." REALPATH)
include_directories(${HDCP_DIR}/common)
include_directories(${HDCP_DIR}/sdk)

add_library(${PROJECT_NAME} 
    SHARED
    hdcpapi.cpp
    session.cpp
    sessionmanager.cpp)

target_compile_options(${PROJECT_NAME} PRIVATE ${HDCP_CXX_FLAGS})
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS ${HDCP_LD_FLAGS})
set_target_properties(${PROJECT_NAME} PROPERTIES
                    VERSION ${HDCP_VERSION} SOVERSION ${HDCP_VERSION_MAJOR})

target_link_libraries(${PROJECT_NAME}
    ${CMAKE_THREAD_LIBS_INIT}
    common
    )
