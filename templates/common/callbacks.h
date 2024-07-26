#ifndef TIBIA_CALLBACKS_H
#define TIBIA_CALLBACKS_H

#include <inttypes.h>

typedef struct {
	void       *handle;
	const char *format;
	const char *(*get_bindir)(void *handle);
	const char *(*get_datadir)(void *handle);
} plugin_callbacks;

typedef struct {
	void       *handle;
	const char *format;

	const char *(*get_bindir)         (void *handle);
	const char *(*get_datadir)        (void *handle);
	void        (*set_parameter_begin)(void *handle, size_t index);
	void        (*set_parameter)      (void *handle, size_t index, float value);
	void        (*set_parameter_end)  (void *handle, size_t index);
} plugin_ui_callbacks;

#endif
