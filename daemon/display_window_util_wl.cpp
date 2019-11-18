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
#include "global_wayland.h"
#include <iostream>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <list>
#include <stdlib.h>

/* IAS Wayland library for IPC calls from hdcpd to IAS compositor *
 * need to set LD_LIBRARY_PATH if library is not in standard path *
 * for example it might be located at /usr/lib64/ias              */
#define IAS_WL_LIBNAME "libwl_base.so"
using namespace std;

typedef struct _class_info_t {
	char *name;
	int level;
} class_info;

typedef enum _wl_stage_t {
	WL_INIT,
	CREATE_DISPLAY_DONE,
} wl_stage;

wl_stage stage = WL_INIT;
static global_wl *gwl;
static void *lib_hdl;
typedef void* (*init_fn)();
typedef void (*deinit_fn)(void *);


EGLNativeDisplayType util_create_display(int screen)
{
	EGLNativeDisplayType ret = 0;
	void *init;

	lib_hdl = dlopen(IAS_WL_LIBNAME, RTLD_LAZY | RTLD_GLOBAL);
	if(!lib_hdl) {
		cerr<<"Couldn't open lib "<<IAS_WL_LIBNAME<<endl;
		return 0;
	}

	/* Find init symbol */
	init = dlsym(lib_hdl, "init");
	if(!init) {
		cerr<<"Couldn't open symbol init dlerror "<<dlerror()<<endl;
		dlclose(lib_hdl);
		return 0;
	}

	/* Get a new class pointer by calling this init function */
	gwl = (global_wl *) ((init_fn) init)();
	if(!gwl) {
		cerr<<"Couldn't get a new class pointer"<<endl;
		dlclose(lib_hdl);
		return 0;
	}

	/* Call class's function */
	ret = gwl->init();
	if(ret == 0) {
		cerr<<"WL base Class init is failed"<<endl;
		dlclose(lib_hdl);
		return 0;
	}
	gwl->add_reg();
	gwl->dispatch_pending();

	stage = CREATE_DISPLAY_DONE;
	return ret;
}

void util_destroy_display(EGLNativeDisplayType display)
{
	void *deinit;

	/* First call the class's deinit function */
	if(gwl) {
		gwl->deinit();
	}

	/* Find deinit symbol */
	deinit = dlsym(lib_hdl, "deinit");
	if(!deinit) {
		cerr<<"Couldn't open symbol deinit dlerror "<<dlerror()<<endl;
	} else {

		/* Call deinit symbol to delete the class pointer */
		((deinit_fn) deinit)(gwl);
	}
	/* Close the handle to the lib */
	dlclose(lib_hdl);

}

bool util_set_content_protection(int crtc, int cp)
{

	bool retval = false;

	if(stage != CREATE_DISPLAY_DONE) {
		cerr<<"Must call util_create_display first"<<endl;
		return retval;
	}

	if(!gwl) {
		cerr<<"Global Wayland class not registered"<<endl;
		return retval;
	}

	retval = gwl->set_content_protection(crtc, cp);
	if (retval != true)
	{
		cerr<<"Failed to set content protection and ret value =%0x"<<retval<<endl;
		return retval;
	}
	return true;
}
