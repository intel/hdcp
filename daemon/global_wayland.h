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

/* For GEM */
#include <libdrm/intel_bufmgr.h>
#include <xf86drm.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <wayland-client.h>
#include <wayland-egl.h>


enum _caps {
	SUPPORT_NONE,
	SUPPORT_DISP_WL,
};

#define MAKE_LEVEL(lvl, caps) (lvl<<4 | caps)

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
		virtual void registry_handle_global(void *data,
				struct wl_registry *registry, uint32_t name,
				const char *interface, uint32_t version);
		virtual void registry_handle_global_remove(void *data,
				struct wl_registry *registry, uint32_t name);
		virtual void set_content_protection(int crtc, int cp);
		static void xdg_shell_ping(void *data, struct zxdg_shell_v6 *shell,
				uint32_t serial);
		static void modelist_event(void *data,
				struct ias_crtc *ias_crtc,
				uint32_t mode_number,
				uint32_t width, uint32_t height, uint32_t refresh);
		static void gamma_event(void *data,
				struct ias_crtc *ias_crtc,
				uint32_t red,
				uint32_t green,
				uint32_t blue);
		static void contrast_event(void *data,
				struct ias_crtc *ias_crtc,
				uint32_t red,
				uint32_t green,
				uint32_t blue);
		static void brightness_event(void *data,
				struct ias_crtc *ias_crtc,
				uint32_t red,
				uint32_t green,
				uint32_t blue);
		static void id_event(void *data,
				struct ias_crtc *ias_crtc,
				uint32_t id);
		static void cp_enabled_event(void *data, struct ias_crtc *ias_crtc);
};

#endif
