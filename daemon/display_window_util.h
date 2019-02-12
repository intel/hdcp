/*
** Copyright 2017 Intel Corporation
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef DISPLAY_WINDOW_UTIL_H
#define DISPLAY_WINDOW_UTIL_H

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


#ifdef __cplusplus
extern "C" {
#endif

EGLNativeDisplayType    util_create_display(int screen);
void                    util_destroy_display(EGLNativeDisplayType display);
bool                    util_set_content_protection(int crtc, int cp);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DISPLAY_WINDOW_UTIL_H */
