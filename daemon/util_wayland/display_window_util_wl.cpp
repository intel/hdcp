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

#include "display_window_util.h"
#include <global_wayland.h>

static global_wl*                      gwl;

util_env_type_t util_get_env_type()
{
	return UTIL_ENV_TYPE_WLD;
}

EGLNativeDisplayType util_create_display(int screen)
{
	EGLNativeDisplayType ret;
	gwl = new global_wl();
	ret = gwl->init();
	if(ret) {
		gwl->add_reg();
	}
	return ret;
}

void util_destroy_display(EGLNativeDisplayType display)
{
	gwl->deinit();
	delete gwl;
}

void util_set_content_protection(int crtc, int cp)
{
	gwl->set_content_protection(crtc, cp);
}
