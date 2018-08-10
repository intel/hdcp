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

typedef enum {
	UTIL_ENV_TYPE_AND = 0,
	UTIL_ENV_TYPE_X11 = 1,
	UTIL_ENV_TYPE_WLD = 2,
	UTIL_ENV_TYPE_DRM = 3,
} util_env_type_t;

util_env_type_t         util_get_env_type();

EGLNativeDisplayType    util_create_display(int screen);
void                    util_destroy_display(EGLNativeDisplayType display);
void                    util_set_content_protection(int crtc, int cp);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* DISPLAY_WINDOW_UTIL_H */
