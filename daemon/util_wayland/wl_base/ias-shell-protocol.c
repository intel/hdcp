/* 
 * Copyright (c) 2012, Intel Corporation.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface ias_surface_interface;
extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_surface_interface;

static const struct wl_interface *types[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&wl_output_interface,
	&ias_surface_interface,
	&ias_surface_interface,
	&ias_surface_interface,
	&ias_surface_interface,
	&ias_surface_interface,
	NULL,
	NULL,
	&wl_output_interface,
	&ias_surface_interface,
	NULL,
	&ias_surface_interface,
	NULL,
	&ias_surface_interface,
	NULL,
	&ias_surface_interface,
	&wl_surface_interface,
	NULL,
	&ias_surface_interface,
	NULL,
	NULL,
	&wl_output_interface,
};

static const struct wl_message ias_shell_requests[] = {
	{ "set_background", "oo", types + 13 },
	{ "set_parent", "oo", types + 15 },
	{ "set_parent_with_position", "ooii", types + 17 },
	{ "popup", "oou", types + 21 },
	{ "set_zorder", "ou", types + 24 },
	{ "set_behavior", "ou", types + 26 },
	{ "get_ias_surface", "nos", types + 28 },
};

static const struct wl_message ias_shell_events[] = {
	{ "configure", "oii", types + 31 },
};

WL_EXPORT const struct wl_interface ias_shell_interface = {
	"ias_shell", 1,
	7, ias_shell_requests,
	1, ias_shell_events,
};

static const struct wl_message ias_surface_requests[] = {
	{ "pong", "u", types + 0 },
	{ "set_title", "s", types + 0 },
	{ "set_fullscreen", "?o", types + 34 },
	{ "unset_fullscreen", "uu", types + 0 },
};

static const struct wl_message ias_surface_events[] = {
	{ "ping", "u", types + 0 },
	{ "configure", "ii", types + 0 },
};

WL_EXPORT const struct wl_interface ias_surface_interface = {
	"ias_surface", 1,
	4, ias_surface_requests,
	2, ias_surface_events,
};

static const struct wl_message ias_hmi_requests[] = {
	{ "set_constant_alpha", "uu", types + 0 },
	{ "move_surface", "uii", types + 0 },
	{ "resize_surface", "uuu", types + 0 },
	{ "zorder_surface", "uu", types + 0 },
	{ "set_visible", "uu", types + 0 },
	{ "set_behavior", "uu", types + 0 },
	{ "set_shareable", "uu", types + 0 },
	{ "get_surface_sharing_info", "u", types + 0 },
	{ "start_capture", "uuii", types + 0 },
	{ "stop_capture", "uu", types + 0 },
	{ "release_buffer_handle", "uuuuu", types + 0 },
};

static const struct wl_message ias_hmi_events[] = {
	{ "surface_info", "usuiiuuuuusuu", types + 0 },
	{ "surface_destroyed", "usus", types + 0 },
	{ "surface_sharing_info", "usuus", types + 0 },
	{ "raw_buffer_handle", "iuuuuuuuuuuu", types + 0 },
	{ "raw_buffer_fd", "huuuuuuuu", types + 0 },
	{ "capture_error", "ii", types + 0 },
};

WL_EXPORT const struct wl_interface ias_hmi_interface = {
	"ias_hmi", 1,
	11, ias_hmi_requests,
	6, ias_hmi_events,
};

