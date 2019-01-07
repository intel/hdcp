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
static list<class_info *> *lib_list;
static void *lib_hdl;

typedef void* (*init_fn)();
typedef void (*deinit_fn)(void *);
typedef double (*level_fn)();

void find_libs(const char *dir_name=".")
{
	DIR *d;
	struct dirent *dir;
	void *lib_hdl;
	void *level;
	class_info *new_node;

	d = opendir(dir_name);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			/* Compare the last 3 characters of a file to match .so */
			if(strlen(dir->d_name) > 3 &&
					!strncmp(&(dir->d_name[strlen(dir->d_name) - 3]), ".so", 3)) {
				/*
				 * Out of these libs, which ones have the symbol level as we need
				 * it.
				 */
				lib_hdl = dlopen(dir->d_name, RTLD_LAZY | RTLD_GLOBAL);
				if(!lib_hdl) {
					cerr<<"Couldn't open lib "<<dir->d_name
						<<" due to "<<dlerror()<<endl;
					continue;
				}

				level = dlsym(lib_hdl, "level");
				if(!level) {
					dlclose(lib_hdl);
					continue;
				}

				new_node = new class_info();
				new_node->name = strdup(dir->d_name);
				/*
				 * Call level symbol to find out where in class hierarchy is
				 * this class
				 */
				new_node->level = ((level_fn) level)();
				lib_list->push_back(new_node);

				dlclose(lib_hdl);
			}
		}
		closedir(d);
	}
}

bool compare(const class_info *first, const class_info *second)
{
  return first->level > second->level;
}

EGLNativeDisplayType util_create_display(int screen)
{
	EGLNativeDisplayType ret = 0;
	lib_list = new list<class_info *>;
	list<class_info *>::iterator it;
	void *init;

	/*
	 * Find libs in the current directory which have an deinit symbol in
	 * them
	 */
	find_libs();

	/* Sort this list based on descending level */
	lib_list->sort(compare);

	/*
	 * Find the first lib which is the highest level because we just sorted
	 * them
	 */
	it = lib_list->begin();
	if(*it) {
		lib_hdl = dlopen((*it)->name, RTLD_LAZY | RTLD_GLOBAL);
		if(!lib_hdl) {
			cerr<<"Couldn't open lib"<<(*it)->name<<endl;
			return 0;
		}

		/* Find init symbol */
		init = dlsym(lib_hdl, "init");
		if(!init) {
			cerr<<"Couldn't open symbol init"<<endl;
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
		gwl->add_reg();
	}

	stage = CREATE_DISPLAY_DONE;
	return ret;
}

void util_destroy_display(EGLNativeDisplayType display)
{
	list<class_info *>::iterator it;
	void *deinit;

	/* First call the class's deinit function */
	if(gwl) {
		gwl->deinit();
	}

	/* Find deinit symbol */
	deinit = dlsym(lib_hdl, "deinit");
	if(!deinit) {
		cerr<<"Couldn't open symbol deinit"<<endl;
	} else {

		/* Call deinit symbol to delete the class pointer */
		((deinit_fn) deinit)(gwl);
	}
	/* Close the handle to the lib */
	dlclose(lib_hdl);

	/* Delete contents of the list first */
	for(it = lib_list->begin(); it != lib_list->end(); it++) {
		free((*it)->name);
		delete *it;
	}

	/* Delete the list */
	delete lib_list;
}

void util_set_content_protection(int crtc, int cp)
{
	if(stage != CREATE_DISPLAY_DONE) {
		cerr<<"Must call util_create_display first"<<endl;
		return;
	}

	if(!gwl) {
		cerr<<"Global Wayland class not registered"<<endl;
		return;
	}

	gwl->set_content_protection(crtc, cp);
}
