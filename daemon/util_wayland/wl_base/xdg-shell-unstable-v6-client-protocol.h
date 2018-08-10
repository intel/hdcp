/* 
 * Copyright © 2008-2013 Kristian Høgsberg
 * Copyright © 2013      Rafael Antognolli
 * Copyright © 2013      Jasper St. Pierre
 * Copyright © 2010-2013 Intel Corporation
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef XDG_SHELL_UNSTABLE_V6_CLIENT_PROTOCOL_H
#define XDG_SHELL_UNSTABLE_V6_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_output;
struct wl_seat;
struct wl_surface;
struct zxdg_popup_v6;
struct zxdg_positioner_v6;
struct zxdg_shell_v6;
struct zxdg_surface_v6;
struct zxdg_toplevel_v6;

extern const struct wl_interface zxdg_shell_v6_interface;
extern const struct wl_interface zxdg_positioner_v6_interface;
extern const struct wl_interface zxdg_surface_v6_interface;
extern const struct wl_interface zxdg_toplevel_v6_interface;
extern const struct wl_interface zxdg_popup_v6_interface;

#ifndef ZXDG_SHELL_V6_ERROR_ENUM
#define ZXDG_SHELL_V6_ERROR_ENUM
enum zxdg_shell_v6_error {
	ZXDG_SHELL_V6_ERROR_ROLE = 0,
	ZXDG_SHELL_V6_ERROR_DEFUNCT_SURFACES = 1,
	ZXDG_SHELL_V6_ERROR_NOT_THE_TOPMOST_POPUP = 2,
	ZXDG_SHELL_V6_ERROR_INVALID_POPUP_PARENT = 3,
	ZXDG_SHELL_V6_ERROR_INVALID_SURFACE_STATE = 4,
	ZXDG_SHELL_V6_ERROR_INVALID_POSITIONER = 5,
};
#endif /* ZXDG_SHELL_V6_ERROR_ENUM */

/**
 * zxdg_shell_v6 - create desktop-style surfaces
 * @ping: check if the client is alive
 *
 * xdg_shell allows clients to turn a wl_surface into a "real window"
 * which can be dragged, resized, stacked, and moved around by the user.
 * Everything about this interface is suited towards traditional desktop
 * environments.
 */
struct zxdg_shell_v6_listener {
	/**
	 * ping - check if the client is alive
	 * @serial: pass this to the pong request
	 *
	 * The ping event asks the client if it's still alive. Pass the
	 * serial specified in the event back to the compositor by sending
	 * a "pong" request back with the specified serial. See
	 * xdg_shell.ping.
	 *
	 * Compositors can use this to determine if the client is still
	 * alive. It's unspecified what will happen if the client doesn't
	 * respond to the ping request, or in what timeframe. Clients
	 * should try to respond in a reasonable amount of time.
	 *
	 * A compositor is free to ping in any way it wants, but a client
	 * must always respond to any xdg_shell object it created.
	 */
	void (*ping)(void *data,
		     struct zxdg_shell_v6 *zxdg_shell_v6,
		     uint32_t serial);
};

static inline int
zxdg_shell_v6_add_listener(struct zxdg_shell_v6 *zxdg_shell_v6,
			   const struct zxdg_shell_v6_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zxdg_shell_v6,
				     (void (**)(void)) listener, data);
}

#define ZXDG_SHELL_V6_DESTROY	0
#define ZXDG_SHELL_V6_CREATE_POSITIONER	1
#define ZXDG_SHELL_V6_GET_XDG_SURFACE	2
#define ZXDG_SHELL_V6_PONG	3

static inline void
zxdg_shell_v6_set_user_data(struct zxdg_shell_v6 *zxdg_shell_v6, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zxdg_shell_v6, user_data);
}

static inline void *
zxdg_shell_v6_get_user_data(struct zxdg_shell_v6 *zxdg_shell_v6)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zxdg_shell_v6);
}

static inline void
zxdg_shell_v6_destroy(struct zxdg_shell_v6 *zxdg_shell_v6)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_shell_v6,
			 ZXDG_SHELL_V6_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zxdg_shell_v6);
}

static inline struct zxdg_positioner_v6 *
zxdg_shell_v6_create_positioner(struct zxdg_shell_v6 *zxdg_shell_v6)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zxdg_shell_v6,
			 ZXDG_SHELL_V6_CREATE_POSITIONER, &zxdg_positioner_v6_interface, NULL);

	return (struct zxdg_positioner_v6 *) id;
}

static inline struct zxdg_surface_v6 *
zxdg_shell_v6_get_xdg_surface(struct zxdg_shell_v6 *zxdg_shell_v6, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zxdg_shell_v6,
			 ZXDG_SHELL_V6_GET_XDG_SURFACE, &zxdg_surface_v6_interface, NULL, surface);

	return (struct zxdg_surface_v6 *) id;
}

static inline void
zxdg_shell_v6_pong(struct zxdg_shell_v6 *zxdg_shell_v6, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_shell_v6,
			 ZXDG_SHELL_V6_PONG, serial);
}

#ifndef ZXDG_POSITIONER_V6_ERROR_ENUM
#define ZXDG_POSITIONER_V6_ERROR_ENUM
enum zxdg_positioner_v6_error {
	ZXDG_POSITIONER_V6_ERROR_INVALID_INPUT = 0,
};
#endif /* ZXDG_POSITIONER_V6_ERROR_ENUM */

#ifndef ZXDG_POSITIONER_V6_ANCHOR_ENUM
#define ZXDG_POSITIONER_V6_ANCHOR_ENUM
enum zxdg_positioner_v6_anchor {
	ZXDG_POSITIONER_V6_ANCHOR_NONE = 0,
	ZXDG_POSITIONER_V6_ANCHOR_TOP = 1,
	ZXDG_POSITIONER_V6_ANCHOR_BOTTOM = 2,
	ZXDG_POSITIONER_V6_ANCHOR_LEFT = 4,
	ZXDG_POSITIONER_V6_ANCHOR_RIGHT = 8,
};
#endif /* ZXDG_POSITIONER_V6_ANCHOR_ENUM */

#ifndef ZXDG_POSITIONER_V6_GRAVITY_ENUM
#define ZXDG_POSITIONER_V6_GRAVITY_ENUM
enum zxdg_positioner_v6_gravity {
	ZXDG_POSITIONER_V6_GRAVITY_NONE = 0,
	ZXDG_POSITIONER_V6_GRAVITY_TOP = 1,
	ZXDG_POSITIONER_V6_GRAVITY_BOTTOM = 2,
	ZXDG_POSITIONER_V6_GRAVITY_LEFT = 4,
	ZXDG_POSITIONER_V6_GRAVITY_RIGHT = 8,
};
#endif /* ZXDG_POSITIONER_V6_GRAVITY_ENUM */

#ifndef ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_ENUM
#define ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_ENUM
/**
 * zxdg_positioner_v6_constraint_adjustment - vertically resize the
 *	surface
 * @ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_NONE: (none)
 * @ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_SLIDE_X: (none)
 * @ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_SLIDE_Y: (none)
 * @ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_FLIP_X: (none)
 * @ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_FLIP_Y: (none)
 * @ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_RESIZE_X: (none)
 * @ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_RESIZE_Y: (none)
 *
 * Resize the surface vertically so that it is completely unconstrained.
 */
enum zxdg_positioner_v6_constraint_adjustment {
	ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_NONE = 0,
	ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_SLIDE_X = 1,
	ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_SLIDE_Y = 2,
	ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_FLIP_X = 4,
	ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_FLIP_Y = 8,
	ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_RESIZE_X = 16,
	ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_RESIZE_Y = 32,
};
#endif /* ZXDG_POSITIONER_V6_CONSTRAINT_ADJUSTMENT_ENUM */

#define ZXDG_POSITIONER_V6_DESTROY	0
#define ZXDG_POSITIONER_V6_SET_SIZE	1
#define ZXDG_POSITIONER_V6_SET_ANCHOR_RECT	2
#define ZXDG_POSITIONER_V6_SET_ANCHOR	3
#define ZXDG_POSITIONER_V6_SET_GRAVITY	4
#define ZXDG_POSITIONER_V6_SET_CONSTRAINT_ADJUSTMENT	5
#define ZXDG_POSITIONER_V6_SET_OFFSET	6

static inline void
zxdg_positioner_v6_set_user_data(struct zxdg_positioner_v6 *zxdg_positioner_v6, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zxdg_positioner_v6, user_data);
}

static inline void *
zxdg_positioner_v6_get_user_data(struct zxdg_positioner_v6 *zxdg_positioner_v6)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zxdg_positioner_v6);
}

static inline void
zxdg_positioner_v6_destroy(struct zxdg_positioner_v6 *zxdg_positioner_v6)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_positioner_v6,
			 ZXDG_POSITIONER_V6_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zxdg_positioner_v6);
}

static inline void
zxdg_positioner_v6_set_size(struct zxdg_positioner_v6 *zxdg_positioner_v6, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_positioner_v6,
			 ZXDG_POSITIONER_V6_SET_SIZE, width, height);
}

static inline void
zxdg_positioner_v6_set_anchor_rect(struct zxdg_positioner_v6 *zxdg_positioner_v6, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_positioner_v6,
			 ZXDG_POSITIONER_V6_SET_ANCHOR_RECT, x, y, width, height);
}

static inline void
zxdg_positioner_v6_set_anchor(struct zxdg_positioner_v6 *zxdg_positioner_v6, uint32_t anchor)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_positioner_v6,
			 ZXDG_POSITIONER_V6_SET_ANCHOR, anchor);
}

static inline void
zxdg_positioner_v6_set_gravity(struct zxdg_positioner_v6 *zxdg_positioner_v6, uint32_t gravity)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_positioner_v6,
			 ZXDG_POSITIONER_V6_SET_GRAVITY, gravity);
}

static inline void
zxdg_positioner_v6_set_constraint_adjustment(struct zxdg_positioner_v6 *zxdg_positioner_v6, uint32_t constraint_adjustment)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_positioner_v6,
			 ZXDG_POSITIONER_V6_SET_CONSTRAINT_ADJUSTMENT, constraint_adjustment);
}

static inline void
zxdg_positioner_v6_set_offset(struct zxdg_positioner_v6 *zxdg_positioner_v6, int32_t x, int32_t y)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_positioner_v6,
			 ZXDG_POSITIONER_V6_SET_OFFSET, x, y);
}

#ifndef ZXDG_SURFACE_V6_ERROR_ENUM
#define ZXDG_SURFACE_V6_ERROR_ENUM
enum zxdg_surface_v6_error {
	ZXDG_SURFACE_V6_ERROR_NOT_CONSTRUCTED = 1,
	ZXDG_SURFACE_V6_ERROR_ALREADY_CONSTRUCTED = 2,
	ZXDG_SURFACE_V6_ERROR_UNCONFIGURED_BUFFER = 3,
};
#endif /* ZXDG_SURFACE_V6_ERROR_ENUM */

/**
 * zxdg_surface_v6 - desktop user interface surface base interface
 * @configure: suggest a surface change
 *
 * An interface that may be implemented by a wl_surface, for
 * implementations that provide a desktop-style user interface.
 *
 * It provides a base set of functionality required to construct user
 * interface elements requiring management by the compositor, such as
 * toplevel windows, menus, etc. The types of functionality are split into
 * xdg_surface roles.
 *
 * Creating an xdg_surface does not set the role for a wl_surface. In order
 * to map an xdg_surface, the client must create a role-specific object
 * using, e.g., get_toplevel, get_popup. The wl_surface for any given
 * xdg_surface can have at most one role, and may not be assigned any role
 * not based on xdg_surface.
 *
 * A role must be assigned before any other requests are made to the
 * xdg_surface object.
 *
 * The client must call wl_surface.commit on the corresponding wl_surface
 * for the xdg_surface state to take effect.
 *
 * Creating an xdg_surface from a wl_surface which has a buffer attached or
 * committed is a client error, and any attempts by a client to attach or
 * manipulate a buffer prior to the first xdg_surface.configure call must
 * also be treated as errors.
 *
 * For a surface to be mapped by the compositor, the following conditions
 * must be met: (1) the client has assigned a xdg_surface based role to the
 * surface, (2) the client has set and committed the xdg_surface state and
 * the role dependent state to the surface and (3) the client has committed
 * a buffer to the surface.
 */
struct zxdg_surface_v6_listener {
	/**
	 * configure - suggest a surface change
	 * @serial: serial of the configure event
	 *
	 * The configure event marks the end of a configure sequence. A
	 * configure sequence is a set of one or more events configuring
	 * the state of the xdg_surface, including the final
	 * xdg_surface.configure event.
	 *
	 * Where applicable, xdg_surface surface roles will during a
	 * configure sequence extend this event as a latched state sent as
	 * events before the xdg_surface.configure event. Such events
	 * should be considered to make up a set of atomically applied
	 * configuration states, where the xdg_surface.configure commits
	 * the accumulated state.
	 *
	 * Clients should arrange their surface for the new states, and
	 * then send an ack_configure request with the serial sent in this
	 * configure event at some point before committing the new surface.
	 *
	 * If the client receives multiple configure events before it can
	 * respond to one, it is free to discard all but the last event it
	 * received.
	 */
	void (*configure)(void *data,
			  struct zxdg_surface_v6 *zxdg_surface_v6,
			  uint32_t serial);
};

static inline int
zxdg_surface_v6_add_listener(struct zxdg_surface_v6 *zxdg_surface_v6,
			     const struct zxdg_surface_v6_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zxdg_surface_v6,
				     (void (**)(void)) listener, data);
}

#define ZXDG_SURFACE_V6_DESTROY	0
#define ZXDG_SURFACE_V6_GET_TOPLEVEL	1
#define ZXDG_SURFACE_V6_GET_POPUP	2
#define ZXDG_SURFACE_V6_SET_WINDOW_GEOMETRY	3
#define ZXDG_SURFACE_V6_ACK_CONFIGURE	4

static inline void
zxdg_surface_v6_set_user_data(struct zxdg_surface_v6 *zxdg_surface_v6, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zxdg_surface_v6, user_data);
}

static inline void *
zxdg_surface_v6_get_user_data(struct zxdg_surface_v6 *zxdg_surface_v6)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zxdg_surface_v6);
}

static inline void
zxdg_surface_v6_destroy(struct zxdg_surface_v6 *zxdg_surface_v6)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_surface_v6,
			 ZXDG_SURFACE_V6_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zxdg_surface_v6);
}

static inline struct zxdg_toplevel_v6 *
zxdg_surface_v6_get_toplevel(struct zxdg_surface_v6 *zxdg_surface_v6)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zxdg_surface_v6,
			 ZXDG_SURFACE_V6_GET_TOPLEVEL, &zxdg_toplevel_v6_interface, NULL);

	return (struct zxdg_toplevel_v6 *) id;
}

static inline struct zxdg_popup_v6 *
zxdg_surface_v6_get_popup(struct zxdg_surface_v6 *zxdg_surface_v6, struct zxdg_surface_v6 *parent, struct zxdg_positioner_v6 *positioner)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zxdg_surface_v6,
			 ZXDG_SURFACE_V6_GET_POPUP, &zxdg_popup_v6_interface, NULL, parent, positioner);

	return (struct zxdg_popup_v6 *) id;
}

static inline void
zxdg_surface_v6_set_window_geometry(struct zxdg_surface_v6 *zxdg_surface_v6, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_surface_v6,
			 ZXDG_SURFACE_V6_SET_WINDOW_GEOMETRY, x, y, width, height);
}

static inline void
zxdg_surface_v6_ack_configure(struct zxdg_surface_v6 *zxdg_surface_v6, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_surface_v6,
			 ZXDG_SURFACE_V6_ACK_CONFIGURE, serial);
}

#ifndef ZXDG_TOPLEVEL_V6_RESIZE_EDGE_ENUM
#define ZXDG_TOPLEVEL_V6_RESIZE_EDGE_ENUM
/**
 * zxdg_toplevel_v6_resize_edge - edge values for resizing
 * @ZXDG_TOPLEVEL_V6_RESIZE_EDGE_NONE: (none)
 * @ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP: (none)
 * @ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM: (none)
 * @ZXDG_TOPLEVEL_V6_RESIZE_EDGE_LEFT: (none)
 * @ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_LEFT: (none)
 * @ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_LEFT: (none)
 * @ZXDG_TOPLEVEL_V6_RESIZE_EDGE_RIGHT: (none)
 * @ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_RIGHT: (none)
 * @ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_RIGHT: (none)
 *
 * These values are used to indicate which edge of a surface is being
 * dragged in a resize operation.
 */
enum zxdg_toplevel_v6_resize_edge {
	ZXDG_TOPLEVEL_V6_RESIZE_EDGE_NONE = 0,
	ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP = 1,
	ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM = 2,
	ZXDG_TOPLEVEL_V6_RESIZE_EDGE_LEFT = 4,
	ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_LEFT = 5,
	ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_LEFT = 6,
	ZXDG_TOPLEVEL_V6_RESIZE_EDGE_RIGHT = 8,
	ZXDG_TOPLEVEL_V6_RESIZE_EDGE_TOP_RIGHT = 9,
	ZXDG_TOPLEVEL_V6_RESIZE_EDGE_BOTTOM_RIGHT = 10,
};
#endif /* ZXDG_TOPLEVEL_V6_RESIZE_EDGE_ENUM */

#ifndef ZXDG_TOPLEVEL_V6_STATE_ENUM
#define ZXDG_TOPLEVEL_V6_STATE_ENUM
/**
 * zxdg_toplevel_v6_state - the surface is now activated
 * @ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED: the surface is maximized
 * @ZXDG_TOPLEVEL_V6_STATE_FULLSCREEN: the surface is fullscreen
 * @ZXDG_TOPLEVEL_V6_STATE_RESIZING: the surface is being resized
 * @ZXDG_TOPLEVEL_V6_STATE_ACTIVATED: the surface is now activated
 *
 * Client window decorations should be painted as if the window is
 * active. Do not assume this means that the window actually has keyboard
 * or pointer focus.
 */
enum zxdg_toplevel_v6_state {
	ZXDG_TOPLEVEL_V6_STATE_MAXIMIZED = 1,
	ZXDG_TOPLEVEL_V6_STATE_FULLSCREEN = 2,
	ZXDG_TOPLEVEL_V6_STATE_RESIZING = 3,
	ZXDG_TOPLEVEL_V6_STATE_ACTIVATED = 4,
};
#endif /* ZXDG_TOPLEVEL_V6_STATE_ENUM */

/**
 * zxdg_toplevel_v6 - toplevel surface
 * @configure: suggest a surface change
 * @close: surface wants to be closed
 *
 * This interface defines an xdg_surface role which allows a surface to,
 * among other things, set window-like properties such as maximize,
 * fullscreen, and minimize, set application-specific metadata like title
 * and id, and well as trigger user interactive operations such as
 * interactive resize and move.
 */
struct zxdg_toplevel_v6_listener {
	/**
	 * configure - suggest a surface change
	 * @width: (none)
	 * @height: (none)
	 * @states: (none)
	 *
	 * This configure event asks the client to resize its toplevel
	 * surface or to change its state. The configured state should not
	 * be applied immediately. See xdg_surface.configure for details.
	 *
	 * The width and height arguments specify a hint to the window
	 * about how its surface should be resized in window geometry
	 * coordinates. See set_window_geometry.
	 *
	 * If the width or height arguments are zero, it means the client
	 * should decide its own window dimension. This may happen when the
	 * compositor needs to configure the state of the surface but
	 * doesn't have any information about any previous or expected
	 * dimension.
	 *
	 * The states listed in the event specify how the width/height
	 * arguments should be interpreted, and possibly how it should be
	 * drawn.
	 *
	 * Clients must send an ack_configure in response to this event.
	 * See xdg_surface.configure and xdg_surface.ack_configure for
	 * details.
	 */
	void (*configure)(void *data,
			  struct zxdg_toplevel_v6 *zxdg_toplevel_v6,
			  int32_t width,
			  int32_t height,
			  struct wl_array *states);
	/**
	 * close - surface wants to be closed
	 *
	 * The close event is sent by the compositor when the user wants
	 * the surface to be closed. This should be equivalent to the user
	 * clicking the close button in client-side decorations, if your
	 * application has any.
	 *
	 * This is only a request that the user intends to close the
	 * window. The client may choose to ignore this request, or show a
	 * dialog to ask the user to save their data, etc.
	 */
	void (*close)(void *data,
		      struct zxdg_toplevel_v6 *zxdg_toplevel_v6);
};

static inline int
zxdg_toplevel_v6_add_listener(struct zxdg_toplevel_v6 *zxdg_toplevel_v6,
			      const struct zxdg_toplevel_v6_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zxdg_toplevel_v6,
				     (void (**)(void)) listener, data);
}

#define ZXDG_TOPLEVEL_V6_DESTROY	0
#define ZXDG_TOPLEVEL_V6_SET_PARENT	1
#define ZXDG_TOPLEVEL_V6_SET_TITLE	2
#define ZXDG_TOPLEVEL_V6_SET_APP_ID	3
#define ZXDG_TOPLEVEL_V6_SHOW_WINDOW_MENU	4
#define ZXDG_TOPLEVEL_V6_MOVE	5
#define ZXDG_TOPLEVEL_V6_RESIZE	6
#define ZXDG_TOPLEVEL_V6_SET_MAX_SIZE	7
#define ZXDG_TOPLEVEL_V6_SET_MIN_SIZE	8
#define ZXDG_TOPLEVEL_V6_SET_MAXIMIZED	9
#define ZXDG_TOPLEVEL_V6_UNSET_MAXIMIZED	10
#define ZXDG_TOPLEVEL_V6_SET_FULLSCREEN	11
#define ZXDG_TOPLEVEL_V6_UNSET_FULLSCREEN	12
#define ZXDG_TOPLEVEL_V6_SET_MINIMIZED	13

static inline void
zxdg_toplevel_v6_set_user_data(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zxdg_toplevel_v6, user_data);
}

static inline void *
zxdg_toplevel_v6_get_user_data(struct zxdg_toplevel_v6 *zxdg_toplevel_v6)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zxdg_toplevel_v6);
}

static inline void
zxdg_toplevel_v6_destroy(struct zxdg_toplevel_v6 *zxdg_toplevel_v6)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zxdg_toplevel_v6);
}

static inline void
zxdg_toplevel_v6_set_parent(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, struct zxdg_toplevel_v6 *parent)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_SET_PARENT, parent);
}

static inline void
zxdg_toplevel_v6_set_title(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, const char *title)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_SET_TITLE, title);
}

static inline void
zxdg_toplevel_v6_set_app_id(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, const char *app_id)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_SET_APP_ID, app_id);
}

static inline void
zxdg_toplevel_v6_show_window_menu(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, struct wl_seat *seat, uint32_t serial, int32_t x, int32_t y)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_SHOW_WINDOW_MENU, seat, serial, x, y);
}

static inline void
zxdg_toplevel_v6_move(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, struct wl_seat *seat, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_MOVE, seat, serial);
}

static inline void
zxdg_toplevel_v6_resize(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, struct wl_seat *seat, uint32_t serial, uint32_t edges)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_RESIZE, seat, serial, edges);
}

static inline void
zxdg_toplevel_v6_set_max_size(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_SET_MAX_SIZE, width, height);
}

static inline void
zxdg_toplevel_v6_set_min_size(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_SET_MIN_SIZE, width, height);
}

static inline void
zxdg_toplevel_v6_set_maximized(struct zxdg_toplevel_v6 *zxdg_toplevel_v6)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_SET_MAXIMIZED);
}

static inline void
zxdg_toplevel_v6_unset_maximized(struct zxdg_toplevel_v6 *zxdg_toplevel_v6)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_UNSET_MAXIMIZED);
}

static inline void
zxdg_toplevel_v6_set_fullscreen(struct zxdg_toplevel_v6 *zxdg_toplevel_v6, struct wl_output *output)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_SET_FULLSCREEN, output);
}

static inline void
zxdg_toplevel_v6_unset_fullscreen(struct zxdg_toplevel_v6 *zxdg_toplevel_v6)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_UNSET_FULLSCREEN);
}

static inline void
zxdg_toplevel_v6_set_minimized(struct zxdg_toplevel_v6 *zxdg_toplevel_v6)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_toplevel_v6,
			 ZXDG_TOPLEVEL_V6_SET_MINIMIZED);
}

#ifndef ZXDG_POPUP_V6_ERROR_ENUM
#define ZXDG_POPUP_V6_ERROR_ENUM
enum zxdg_popup_v6_error {
	ZXDG_POPUP_V6_ERROR_INVALID_GRAB = 0,
};
#endif /* ZXDG_POPUP_V6_ERROR_ENUM */

/**
 * zxdg_popup_v6 - short-lived, popup surfaces for menus
 * @configure: configure the popup surface
 * @popup_done: popup interaction is done
 *
 * A popup surface is a short-lived, temporary surface. It can be used to
 * implement for example menus, popovers, tooltips and other similar user
 * interface concepts.
 *
 * A popup can be made to take an explicit grab. See xdg_popup.grab for
 * details.
 *
 * When the popup is dismissed, a popup_done event will be sent out, and at
 * the same time the surface will be unmapped. See the xdg_popup.popup_done
 * event for details.
 *
 * Explicitly destroying the xdg_popup object will also dismiss the popup
 * and unmap the surface. Clients that want to dismiss the popup when
 * another surface of their own is clicked should dismiss the popup using
 * the destroy request.
 *
 * The parent surface must have either the xdg_toplevel or xdg_popup
 * surface role.
 *
 * A newly created xdg_popup will be stacked on top of all previously
 * created xdg_popup surfaces associated with the same xdg_toplevel.
 *
 * The parent of an xdg_popup must be mapped (see the xdg_surface
 * description) before the xdg_popup itself.
 *
 * The x and y arguments passed when creating the popup object specify
 * where the top left of the popup should be placed, relative to the local
 * surface coordinates of the parent surface. See xdg_surface.get_popup. An
 * xdg_popup must intersect with or be at least partially adjacent to its
 * parent surface.
 *
 * The client must call wl_surface.commit on the corresponding wl_surface
 * for the xdg_popup state to take effect.
 */
struct zxdg_popup_v6_listener {
	/**
	 * configure - configure the popup surface
	 * @x: x position relative to parent surface window geometry
	 * @y: y position relative to parent surface window geometry
	 * @width: window geometry width
	 * @height: window geometry height
	 *
	 * This event asks the popup surface to configure itself given
	 * the configuration. The configured state should not be applied
	 * immediately. See xdg_surface.configure for details.
	 *
	 * The x and y arguments represent the position the popup was
	 * placed at given the xdg_positioner rule, relative to the upper
	 * left corner of the window geometry of the parent surface.
	 */
	void (*configure)(void *data,
			  struct zxdg_popup_v6 *zxdg_popup_v6,
			  int32_t x,
			  int32_t y,
			  int32_t width,
			  int32_t height);
	/**
	 * popup_done - popup interaction is done
	 *
	 * The popup_done event is sent out when a popup is dismissed by
	 * the compositor. The client should destroy the xdg_popup object
	 * at this point.
	 */
	void (*popup_done)(void *data,
			   struct zxdg_popup_v6 *zxdg_popup_v6);
};

static inline int
zxdg_popup_v6_add_listener(struct zxdg_popup_v6 *zxdg_popup_v6,
			   const struct zxdg_popup_v6_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zxdg_popup_v6,
				     (void (**)(void)) listener, data);
}

#define ZXDG_POPUP_V6_DESTROY	0
#define ZXDG_POPUP_V6_GRAB	1

static inline void
zxdg_popup_v6_set_user_data(struct zxdg_popup_v6 *zxdg_popup_v6, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zxdg_popup_v6, user_data);
}

static inline void *
zxdg_popup_v6_get_user_data(struct zxdg_popup_v6 *zxdg_popup_v6)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zxdg_popup_v6);
}

static inline void
zxdg_popup_v6_destroy(struct zxdg_popup_v6 *zxdg_popup_v6)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_popup_v6,
			 ZXDG_POPUP_V6_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zxdg_popup_v6);
}

static inline void
zxdg_popup_v6_grab(struct zxdg_popup_v6 *zxdg_popup_v6, struct wl_seat *seat, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) zxdg_popup_v6,
			 ZXDG_POPUP_V6_GRAB, seat, serial);
}

#ifdef  __cplusplus
}
#endif

#endif
