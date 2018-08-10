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

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-cursor.h>
#include "global_wayland.h"

#define BATCH_SIZE 0x80000

extern "C" {

double level() {
	return MAKE_LEVEL(1, SUPPORT_NONE);
}

void *init() {
	return (void *) new global_wl;
}

void deinit(void *var) {
	delete (global_wl *)var;
}

}

global_wl::global_wl()
{
	display = NULL;
	registry = NULL;
	compositor = NULL;
	ias_shell = NULL;
	wl_shell = NULL;
	ivi_application = NULL;
}

/*******************************************************************************
 * Description
 *	This is a callback handler from the compositor for xdg shell ping. Upon
 *	receiving a ping call from the compositor, this app must respond back with
 *	a pong.
 * Parameters
 *	void *data - a pointer to global_wl
 *	struct zxdg_shell_v6 *shell - a pointer to shell
 *	uint32_t serial - serial of the ping event
 * Return val
 *	void
 ******************************************************************************/
void
global_wl::xdg_shell_ping(void *data, struct zxdg_shell_v6 *shell, uint32_t serial)
{
	zxdg_shell_v6_pong(shell, serial);
}

static const struct zxdg_shell_v6_listener xdg_shell_listener = {
	global_wl::xdg_shell_ping,
};

void global_wl::modelist_event(void *data,
		struct ias_crtc *ias_crtc,
		uint32_t mode_number,
		uint32_t width, uint32_t height, uint32_t refresh)
{

}

void global_wl::gamma_event(void *data,
		struct ias_crtc *ias_crtc,
		uint32_t red,
		uint32_t green,
		uint32_t blue)
{

}

void global_wl::contrast_event(void *data,
		struct ias_crtc *ias_crtc,
		uint32_t red,
		uint32_t green,
		uint32_t blue)
{

}

void global_wl::brightness_event(void *data,
		struct ias_crtc *ias_crtc,
		uint32_t red,
		uint32_t green,
		uint32_t blue)
{

}

void global_wl::id_event(void *data,
	   struct ias_crtc *ias_crtc,
	   uint32_t id)
{
	struct crtc *crtc = (struct crtc *) data;

	crtc->id = id;
}

void global_wl::cp_enabled_event(void *data, struct ias_crtc *ias_crtc)
{

}

static const struct ias_crtc_listener ias_crtc_listener = {
	global_wl::modelist_event,
	global_wl::gamma_event,
	global_wl::contrast_event,
	global_wl::brightness_event,
	global_wl::id_event,
	global_wl::cp_enabled_event,
};

/*******************************************************************************
 * Description
 *	This function handles the global registry handler. It is called at
 *	initialization time when a new interface is being advertised to the app.
 *	It cares about the wl_compositor and various shell interfaces that the IAS
 *	compositor handles. It is implemented in the base class and may also be
 *	called by inherited classes.
 * Parameters
 *	void *data - a pointer to global_wl
 *	struct wl_registry *registry - a pointer to the Wayland registry
 *	uint32_t name - The id of the interface
 *	const char *interface - The name of the interface
 *	uint32_t version - The version number for this interface
 * Return val
 *	void
 ******************************************************************************/
void
global_wl::registry_handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	global_wl *d = (global_wl *) data;
	struct crtc *c;

	if (strcmp(interface, "wl_compositor") == 0) {
		d->compositor = (wl_compositor *)
			wl_registry_bind(registry, name,
					&wl_compositor_interface, 1);
	} else if (strcmp(interface, "zxdg_shell_v6") == 0) {
		if (!d->ias_shell && !d->ivi_application) {
			d->wl_shell = (struct zxdg_shell_v6 *) wl_registry_bind(registry,
					name, &zxdg_shell_v6_interface, 1);
			zxdg_shell_v6_add_listener(d->wl_shell, &xdg_shell_listener, d);
		}
	} else if (strcmp(interface, "ias_shell") == 0) {
		if (!d->wl_shell && !d->ivi_application) {
			d->ias_shell = (struct ias_shell *) wl_registry_bind(registry, name,
					&ias_shell_interface, 1);
		}
	} else if (strcmp(interface, "ivi_application") == 0) {
		if (!d->ias_shell && !d->wl_shell) {
			d->ivi_application = (struct ivi_application *) wl_registry_bind(
					registry, name, &ivi_application_interface, 1);
		}
	} else if (strcmp(interface, "ias_crtc") == 0) {
		c = (struct crtc *) calloc(1, sizeof *c);
		if (!c) {
			printf("Error: Failed to handle global event: out of memory\n");
			return;
		}

		wl_list_insert(&d->crtc_list, &c->link);

		c->ias_crtc = (struct ias_crtc *) wl_registry_bind(
				registry, name, &ias_crtc_interface, 1);
		ias_crtc_add_listener(c->ias_crtc, &ias_crtc_listener, c);
	}
}

/*******************************************************************************
 * Description
 *	This function handles the global registry handler remove function. It is
 *	called when an interface is being removed by the compositor.
 * Parameters
 *	void *data - a pointer to global_wl
 *	struct wl_registry *registry - a pointer to the Wayland registry
 *	uint32_t name - The id of the interface
 * Return val
 *	void
 ******************************************************************************/
void
global_wl::registry_handle_global_remove(void *data, struct wl_registry *registry,
			      uint32_t name)
{
}

/*******************************************************************************
 * Description
 *	A static function that calls the wl_global classes corresponding virtual
 *	function
 * Parameters
 *	void *data - a pointer to global_wl
 *	struct wl_registry *registry - a pointer to the Wayland registry
 *	uint32_t name - The id of the interface
 *	const char *interface - The name of the interface
 *	uint32_t version - The version number for this interface
 * Return val
 *	void
 ******************************************************************************/
static void rhg(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	((global_wl *) data)->registry_handle_global(data, registry, name,
			interface, version);
}

/*******************************************************************************
 * Description
 *	A static function that calls the wl_global classes corresponding virtual
 *	function
 * Parameters
 *	void *data - a pointer to global_wl
 *	struct wl_registry *registry - a pointer to the Wayland registry
 *	uint32_t name - The id of the interface
 * Return val
 *	void
 ******************************************************************************/
static void rhgr(void *data, struct wl_registry *registry,
	      uint32_t name)
{
	((global_wl *) data)->registry_handle_global_remove(data, registry, name);
}

static const struct wl_registry_listener registry_listener = {
	rhg,
	rhgr,
};

/*******************************************************************************
 * Description
 *	This function is called to start communicating with the compositor. It calls
 *	key functions like wl_display_init and wl_display_get_registry.
 * Parameters
 *	None
 * Return val
 *	EGLNativeDisplayType
 ******************************************************************************/
EGLNativeDisplayType global_wl::init()
{
	display = wl_display_connect(NULL);
	if(!display) {
		printf("Error: Couldn't connect to Wayland socket\n");
		return 0;
	}

	registry = wl_display_get_registry(display);
	if(!registry) {
		printf("Error: Couldn't get Wayland registry\n");
		return 0;
	}

	wl_list_init(&crtc_list);

	return (EGLNativeDisplayType) display;
}

/*******************************************************************************
 * Description
 *	This function adds a global registry listener.
 * Parameters
 *	None
 * Return val
 *	void
 ******************************************************************************/
void global_wl::add_reg()
{
	wl_registry_add_listener(registry, &registry_listener, this);
	wl_display_roundtrip(display);
	wl_display_roundtrip(display);
}

/*******************************************************************************
 * Description
 *	This function deinitializes all Wayland related functionality and should be
 *	called when a client app no longer wishes to communicate with the compositor
 * Parameters
 *	None
 * Return val
 *	void
 ******************************************************************************/
void global_wl::deinit()
{
	struct crtc *crtc, *temp_crtc;

	if (wl_shell) {
		zxdg_shell_v6_destroy(wl_shell);
	}

	if (ivi_application) {
		ivi_application_destroy(ivi_application);
	}

	if (ias_shell) {
		ias_shell_destroy(ias_shell);
	}

	if (compositor) {
		wl_compositor_destroy(compositor);
	}

	wl_list_for_each_safe(crtc, temp_crtc, &crtc_list, link) {
		ias_crtc_destroy(crtc->ias_crtc);
		wl_list_remove(&crtc->link);
		free(crtc);
	}

	wl_registry_destroy(registry);
	wl_display_flush(display);
	wl_display_disconnect(display);
}

/*******************************************************************************
 * Description
 *	This function peeks at the Wayland event queue to find if there are any
 *	events from the compositor or if there are any requests from the client that
 *	need to be flushed to the compositor. It is a non blocking call which means
 *	that if there aren't any requests/events, it returns immediately.
 * Parameters
 *	None
 * Return val
 *	void
 ******************************************************************************/
void global_wl::dispatch_pending()
{
	wl_display_dispatch_pending(display);
}

/*******************************************************************************
 * Description
 *	This function sets the content protection property on a display connector.
 *	It requires the crtc id of the connector as well as a flag which specifies
 *	whether to turn content protection on or off.
 * Parameters
 *	int crtc_id - The CRTC to set content protection for
 *	int cp - 0/1 flag indicating to turn contention protection off/on 
 *		respectively.
 * Return val
 *	void
 ******************************************************************************/
void global_wl::set_content_protection(int crtc_id, int cp)
{
	struct crtc *crtc;

	wl_list_for_each(crtc, &crtc_list, link) {
		if(crtc->id == (uint32_t) crtc_id) {
			ias_crtc_set_content_protection(crtc->ias_crtc, cp);
			break;
		}
	}
	wl_display_roundtrip(display);
}
