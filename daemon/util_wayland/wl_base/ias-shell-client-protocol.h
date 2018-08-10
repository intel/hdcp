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

#ifndef IASSHELL_CLIENT_PROTOCOL_H
#define IASSHELL_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct ias_hmi;
struct ias_shell;
struct ias_surface;
struct wl_output;
struct wl_surface;

extern const struct wl_interface ias_shell_interface;
extern const struct wl_interface ias_surface_interface;
extern const struct wl_interface ias_hmi_interface;

/**
 * ias_shell - IVI shell interface
 * @configure: Surface resize
 *
 * This interface provides the IVI-specific shell functionality exposed
 * by the Intel Automotive Solutions shell.
 */
struct ias_shell_listener {
	/**
	 * configure - Surface resize
	 * @surface: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 * This provides a standard surface resize notification, exactly
	 * the same as what desktop_shell provides, except there is no
	 * notion of edges since IVI doesn't have a mouse interface for
	 * resizing windows like desktop does.
	 */
	void (*configure)(void *data,
			  struct ias_shell *ias_shell,
			  struct ias_surface *surface,
			  int32_t width,
			  int32_t height);
};

static inline int
ias_shell_add_listener(struct ias_shell *ias_shell,
		       const struct ias_shell_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) ias_shell,
				     (void (**)(void)) listener, data);
}

#define IAS_SHELL_SET_BACKGROUND	0
#define IAS_SHELL_SET_PARENT	1
#define IAS_SHELL_SET_PARENT_WITH_POSITION	2
#define IAS_SHELL_POPUP	3
#define IAS_SHELL_SET_ZORDER	4
#define IAS_SHELL_SET_BEHAVIOR	5
#define IAS_SHELL_GET_IAS_SURFACE	6

static inline void
ias_shell_set_user_data(struct ias_shell *ias_shell, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) ias_shell, user_data);
}

static inline void *
ias_shell_get_user_data(struct ias_shell *ias_shell)
{
	return wl_proxy_get_user_data((struct wl_proxy *) ias_shell);
}

static inline void
ias_shell_destroy(struct ias_shell *ias_shell)
{
	wl_proxy_destroy((struct wl_proxy *) ias_shell);
}

static inline void
ias_shell_set_background(struct ias_shell *ias_shell, struct wl_output *output, struct ias_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) ias_shell,
			 IAS_SHELL_SET_BACKGROUND, output, surface);
}

static inline void
ias_shell_set_parent(struct ias_shell *ias_shell, struct ias_surface *surface, struct ias_surface *parent)
{
	wl_proxy_marshal((struct wl_proxy *) ias_shell,
			 IAS_SHELL_SET_PARENT, surface, parent);
}

static inline void
ias_shell_set_parent_with_position(struct ias_shell *ias_shell, struct ias_surface *surface, struct ias_surface *parent, int32_t x, int32_t y)
{
	wl_proxy_marshal((struct wl_proxy *) ias_shell,
			 IAS_SHELL_SET_PARENT_WITH_POSITION, surface, parent, x, y);
}

static inline void
ias_shell_popup(struct ias_shell *ias_shell, struct wl_output *output, struct ias_surface *surface, uint32_t priority)
{
	wl_proxy_marshal((struct wl_proxy *) ias_shell,
			 IAS_SHELL_POPUP, output, surface, priority);
}

static inline void
ias_shell_set_zorder(struct ias_shell *ias_shell, struct ias_surface *surface, uint32_t zorder)
{
	wl_proxy_marshal((struct wl_proxy *) ias_shell,
			 IAS_SHELL_SET_ZORDER, surface, zorder);
}

static inline void
ias_shell_set_behavior(struct ias_shell *ias_shell, struct ias_surface *surface, uint32_t behavior)
{
	wl_proxy_marshal((struct wl_proxy *) ias_shell,
			 IAS_SHELL_SET_BEHAVIOR, surface, behavior);
}

static inline struct ias_surface *
ias_shell_get_ias_surface(struct ias_shell *ias_shell, struct wl_surface *surface, const char *name)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) ias_shell,
			 IAS_SHELL_GET_IAS_SURFACE, &ias_surface_interface, NULL, surface, name);

	return (struct ias_surface *) id;
}

/**
 * ias_surface - IAS shell surface interface
 * @ping: ping client
 * @configure: suggest resize
 *
 * An interface implemented by a wl_surface. On server side the object is
 * automatically destroyed when the related wl_surface is destroyed. On
 * client side, ias_surface_destroy() must be called before destroying the
 * wl_surface object.
 */
struct ias_surface_listener {
	/**
	 * ping - ping client
	 * @serial: (none)
	 *
	 * Ping a client to check if it is receiving events and sending
	 * requests. A client is expected to reply with a pong request.
	 */
	void (*ping)(void *data,
		     struct ias_surface *ias_surface,
		     uint32_t serial);
	/**
	 * configure - suggest resize
	 * @width: (none)
	 * @height: (none)
	 *
	 * The configure event asks the client to resize its surface. The
	 * size is a hint, in the sense that the client is free to ignore
	 * it if it doesn't resize, pick a smaller size (to satisfy aspect
	 * ratio or resize in steps of NxM pixels). The client is free to
	 * dismiss all but the last configure event it received.
	 */
	void (*configure)(void *data,
			  struct ias_surface *ias_surface,
			  int32_t width,
			  int32_t height);
};

static inline int
ias_surface_add_listener(struct ias_surface *ias_surface,
			 const struct ias_surface_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) ias_surface,
				     (void (**)(void)) listener, data);
}

#define IAS_SURFACE_PONG	0
#define IAS_SURFACE_SET_TITLE	1
#define IAS_SURFACE_SET_FULLSCREEN	2
#define IAS_SURFACE_UNSET_FULLSCREEN	3

static inline void
ias_surface_set_user_data(struct ias_surface *ias_surface, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) ias_surface, user_data);
}

static inline void *
ias_surface_get_user_data(struct ias_surface *ias_surface)
{
	return wl_proxy_get_user_data((struct wl_proxy *) ias_surface);
}

static inline void
ias_surface_destroy(struct ias_surface *ias_surface)
{
	wl_proxy_destroy((struct wl_proxy *) ias_surface);
}

static inline void
ias_surface_pong(struct ias_surface *ias_surface, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) ias_surface,
			 IAS_SURFACE_PONG, serial);
}

static inline void
ias_surface_set_title(struct ias_surface *ias_surface, const char *title)
{
	wl_proxy_marshal((struct wl_proxy *) ias_surface,
			 IAS_SURFACE_SET_TITLE, title);
}

static inline void
ias_surface_set_fullscreen(struct ias_surface *ias_surface, struct wl_output *output)
{
	wl_proxy_marshal((struct wl_proxy *) ias_surface,
			 IAS_SURFACE_SET_FULLSCREEN, output);
}

static inline void
ias_surface_unset_fullscreen(struct ias_surface *ias_surface, uint32_t width, uint32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) ias_surface,
			 IAS_SURFACE_UNSET_FULLSCREEN, width, height);
}

#ifndef IAS_HMI_VISIBLE_OPTIONS_ENUM
#define IAS_HMI_VISIBLE_OPTIONS_ENUM
enum ias_hmi_visible_options {
	IAS_HMI_VISIBLE_OPTIONS_HIDDEN = 0,
	IAS_HMI_VISIBLE_OPTIONS_VISIBLE = 1,
};
#endif /* IAS_HMI_VISIBLE_OPTIONS_ENUM */

#ifndef IAS_HMI_FCAP_ERROR_ENUM
#define IAS_HMI_FCAP_ERROR_ENUM
enum ias_hmi_FCAP_ERROR {
	IAS_HMI_FCAP_ERROR_OK = 0,
	IAS_HMI_FCAP_ERROR_NO_CAPTURE_PROXY = 1,
	IAS_HMI_FCAP_ERROR_DUPLICATE = 2,
	IAS_HMI_FCAP_ERROR_NOT_BUILT_IN = 3,
	IAS_HMI_FCAP_ERROR_INVALID = 4,
};
#endif /* IAS_HMI_FCAP_ERROR_ENUM */

/**
 * ias_hmi - IVI HMI interface
 * @surface_info: Notifies listeners of surface changes
 * @surface_destroyed: Notifies listeners of surface destruction
 * @surface_sharing_info: Notifies listeners of surface sharing changes
 * @raw_buffer_handle: Send weston buffer handle to client
 * @raw_buffer_fd: Send weston buffer prime fd to client
 * @capture_error: Send capture error notification to client
 *
 * This interface provides a client application to control other
 * application's surfaces.
 */
struct ias_hmi_listener {
	/**
	 * surface_info - Notifies listeners of surface changes
	 * @id: (none)
	 * @title: (none)
	 * @z_order: (none)
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 * @alpha: (none)
	 * @behavior: (none)
	 * @pid: (none)
	 * @pname: (none)
	 * @output: (none)
	 * @flipped: (none)
	 *
	 * Notifies clients listening on the ias_hmi interface that a
	 * surface has been created or modified.
	 */
	void (*surface_info)(void *data,
			     struct ias_hmi *ias_hmi,
			     uint32_t id,
			     const char *title,
			     uint32_t z_order,
			     int32_t x,
			     int32_t y,
			     uint32_t width,
			     uint32_t height,
			     uint32_t alpha,
			     uint32_t behavior,
			     uint32_t pid,
			     const char *pname,
			     uint32_t output,
			     uint32_t flipped);
	/**
	 * surface_destroyed - Notifies listeners of surface destruction
	 * @id: (none)
	 * @title: (none)
	 * @pid: (none)
	 * @pname: (none)
	 *
	 * Notifies clients listening on the ias_hmi interface that an
	 * existing surface has been destroyed, allowing them to clean up
	 * their internal bookkeeping.
	 */
	void (*surface_destroyed)(void *data,
				  struct ias_hmi *ias_hmi,
				  uint32_t id,
				  const char *title,
				  uint32_t pid,
				  const char *pname);
	/**
	 * surface_sharing_info - Notifies listeners of surface sharing
	 *	changes
	 * @id: (none)
	 * @title: (none)
	 * @shareable: (none)
	 * @pid: (none)
	 * @pname: (none)
	 *
	 * Notifies clients listening on the ias_hmi interface that a
	 * surface sharing has been enabled or disabled.
	 */
	void (*surface_sharing_info)(void *data,
				     struct ias_hmi *ias_hmi,
				     uint32_t id,
				     const char *title,
				     uint32_t shareable,
				     uint32_t pid,
				     const char *pname);
	/**
	 * raw_buffer_handle - Send weston buffer handle to client
	 * @handle: (none)
	 * @timestamp: (none)
	 * @frame_number: (none)
	 * @stride0: (none)
	 * @stride1: (none)
	 * @stride2: (none)
	 * @format: (none)
	 * @out_width: (none)
	 * @out_height: (none)
	 * @shm_surf_id: (none)
	 * @buf_id: (none)
	 * @image_id: (none)
	 *
	 * Send raw weston buffers to a client that has registered an
	 * interest in the buffers for a particular surface or output. The
	 * client will need to know the stride of each plane.
	 */
	void (*raw_buffer_handle)(void *data,
				  struct ias_hmi *ias_hmi,
				  int32_t handle,
				  uint32_t timestamp,
				  uint32_t frame_number,
				  uint32_t stride0,
				  uint32_t stride1,
				  uint32_t stride2,
				  uint32_t format,
				  uint32_t out_width,
				  uint32_t out_height,
				  uint32_t shm_surf_id,
				  uint32_t buf_id,
				  uint32_t image_id);
	/**
	 * raw_buffer_fd - Send weston buffer prime fd to client
	 * @prime_fd: (none)
	 * @timestamp: (none)
	 * @frame_number: (none)
	 * @stride0: (none)
	 * @stride1: (none)
	 * @stride2: (none)
	 * @format: (none)
	 * @out_width: (none)
	 * @out_height: (none)
	 *
	 * Send raw weston buffers to a client that has registered an
	 * interest in the buffers for a particular surface or output. The
	 * client will need to know the stride of each plane.
	 */
	void (*raw_buffer_fd)(void *data,
			      struct ias_hmi *ias_hmi,
			      int32_t prime_fd,
			      uint32_t timestamp,
			      uint32_t frame_number,
			      uint32_t stride0,
			      uint32_t stride1,
			      uint32_t stride2,
			      uint32_t format,
			      uint32_t out_width,
			      uint32_t out_height);
	/**
	 * capture_error - Send capture error notification to client
	 * @pid: (none)
	 * @error: (none)
	 *
	 * The client needs to be informed of any errors that may occur
	 * during creation or operation of the frame capture. We can send
	 * error codes through here to prompt shutdown of the client
	 * application, should the error be fatal.
	 */
	void (*capture_error)(void *data,
			      struct ias_hmi *ias_hmi,
			      int32_t pid,
			      int32_t error);
};

static inline int
ias_hmi_add_listener(struct ias_hmi *ias_hmi,
		     const struct ias_hmi_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) ias_hmi,
				     (void (**)(void)) listener, data);
}

#define IAS_HMI_SET_CONSTANT_ALPHA	0
#define IAS_HMI_MOVE_SURFACE	1
#define IAS_HMI_RESIZE_SURFACE	2
#define IAS_HMI_ZORDER_SURFACE	3
#define IAS_HMI_SET_VISIBLE	4
#define IAS_HMI_SET_BEHAVIOR	5
#define IAS_HMI_SET_SHAREABLE	6
#define IAS_HMI_GET_SURFACE_SHARING_INFO	7
#define IAS_HMI_START_CAPTURE	8
#define IAS_HMI_STOP_CAPTURE	9
#define IAS_HMI_RELEASE_BUFFER_HANDLE	10

static inline void
ias_hmi_set_user_data(struct ias_hmi *ias_hmi, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) ias_hmi, user_data);
}

static inline void *
ias_hmi_get_user_data(struct ias_hmi *ias_hmi)
{
	return wl_proxy_get_user_data((struct wl_proxy *) ias_hmi);
}

static inline void
ias_hmi_destroy(struct ias_hmi *ias_hmi)
{
	wl_proxy_destroy((struct wl_proxy *) ias_hmi);
}

static inline void
ias_hmi_set_constant_alpha(struct ias_hmi *ias_hmi, uint32_t id, uint32_t alpha)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_SET_CONSTANT_ALPHA, id, alpha);
}

static inline void
ias_hmi_move_surface(struct ias_hmi *ias_hmi, uint32_t id, int32_t x, int32_t y)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_MOVE_SURFACE, id, x, y);
}

static inline void
ias_hmi_resize_surface(struct ias_hmi *ias_hmi, uint32_t id, uint32_t width, uint32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_RESIZE_SURFACE, id, width, height);
}

static inline void
ias_hmi_zorder_surface(struct ias_hmi *ias_hmi, uint32_t id, uint32_t zorder)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_ZORDER_SURFACE, id, zorder);
}

static inline void
ias_hmi_set_visible(struct ias_hmi *ias_hmi, uint32_t id, uint32_t visible)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_SET_VISIBLE, id, visible);
}

static inline void
ias_hmi_set_behavior(struct ias_hmi *ias_hmi, uint32_t id, uint32_t behavior_bits)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_SET_BEHAVIOR, id, behavior_bits);
}

static inline void
ias_hmi_set_shareable(struct ias_hmi *ias_hmi, uint32_t id, uint32_t shareable)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_SET_SHAREABLE, id, shareable);
}

static inline void
ias_hmi_get_surface_sharing_info(struct ias_hmi *ias_hmi, uint32_t id)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_GET_SURFACE_SHARING_INFO, id);
}

static inline void
ias_hmi_start_capture(struct ias_hmi *ias_hmi, uint32_t surfid, uint32_t output_number, int32_t profile, int32_t verbose)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_START_CAPTURE, surfid, output_number, profile, verbose);
}

static inline void
ias_hmi_stop_capture(struct ias_hmi *ias_hmi, uint32_t surfid, uint32_t output_number)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_STOP_CAPTURE, surfid, output_number);
}

static inline void
ias_hmi_release_buffer_handle(struct ias_hmi *ias_hmi, uint32_t shm_surf_id, uint32_t buf_id, uint32_t image_id, uint32_t surfid, uint32_t output_number)
{
	wl_proxy_marshal((struct wl_proxy *) ias_hmi,
			 IAS_HMI_RELEASE_BUFFER_HANDLE, shm_surf_id, buf_id, image_id, surfid, output_number);
}

#ifdef  __cplusplus
}
#endif

#endif
