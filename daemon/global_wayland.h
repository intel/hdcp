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


#ifndef _GLOBAL_WAYLAND_H_
#define _GLOBAL_WAYLAND_H_


#include <wayland-client.h>
#include <wayland-egl.h>


struct crtc {
	struct ias_crtc *ias_crtc;
	uint32_t id;
	struct wl_list link;
};

class global_wl {
	protected:
		/* To be initialized once i.e. global */
		struct wl_display *display;
		struct wl_registry *registry;
		struct wl_compositor *compositor;
		void *ias_shell;
		void *wl_shell;
		void *ivi_application;
		struct wl_list crtc_list;

	public:
		global_wl();
		virtual ~global_wl() { }
		virtual EGLNativeDisplayType init();
		virtual void deinit();
		virtual void dispatch_pending();
		virtual void add_reg();
		/* adding dummy functions to map with actual global_wl class */
		virtual void dummy1();
		virtual void dummy2();
		virtual bool set_content_protection(int crtc, int cp);
};

#endif
