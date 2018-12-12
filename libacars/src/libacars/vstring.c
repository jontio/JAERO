/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <limits.h>			// INT_MAX
#include <stdio.h>			// vsnprintf
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>			// memcpy
#include <libacars/macros.h>		// la_assert, la_debug_print
#include <libacars/util.h>		// LA_XCALLOC, LA_XFREE
#include <libacars/vstring.h>		// la_vstring

#define LA_VSTR_INITIAL_SIZE 256
#define LA_VSTR_SIZE_MULT 2
#define LA_VSTR_SIZE_MAX INT_MAX

static void la_vstring_grow(la_vstring * const vstr, size_t const space_needed) {
	la_assert(vstr);

	size_t new_size = vstr->allocated_size;
	while(vstr->len + space_needed >= new_size) {
		new_size *= LA_VSTR_SIZE_MULT;
	}
	la_debug_print("allocated_size=%zu len=%zu space_needed=%zu new_size: %zu\n",
		vstr->allocated_size, vstr->len, space_needed, new_size);
	la_assert(new_size <= LA_VSTR_SIZE_MAX);
	vstr->str = LA_XREALLOC(vstr->str, new_size);
	vstr->allocated_size = new_size;
}

static size_t la_vstring_space_left(la_vstring const * const vstr) {
	la_assert(vstr);
	la_assert(vstr->allocated_size >= vstr->len);
	return vstr->allocated_size - vstr->len;
}

la_vstring *la_vstring_new() {
	la_vstring *vstr = LA_XCALLOC(1, sizeof(la_vstring));
	vstr->str = LA_XCALLOC(LA_VSTR_INITIAL_SIZE, sizeof(char));
	vstr->allocated_size = LA_VSTR_INITIAL_SIZE;
	vstr->len = 0;
	return vstr;
}

void la_vstring_destroy(la_vstring *vstr, bool destroy_buffer) {
	if(vstr && destroy_buffer == true) {
		LA_XFREE(vstr->str);
	}
	LA_XFREE(vstr);
}

void la_vstring_append_sprintf(la_vstring * const vstr, char const *fmt, ...) {
	la_assert(vstr);
	la_assert(fmt);

	size_t space_left = la_vstring_space_left(vstr);
	size_t result_size;
	int ret;
	va_list ap;
	va_start(ap, fmt);
		ret = vsnprintf(vstr->str + vstr->len, space_left, fmt, ap);
	va_end(ap);
	la_assert(ret >= 0);
	result_size = 1 + (size_t)ret;
	if(result_size < space_left) {	// we have enough space
		goto end;
	} else {
		// Not enough space - realloc and retry once
		la_vstring_grow(vstr, result_size);
		space_left = la_vstring_space_left(vstr);
		va_start(ap, fmt);
			ret = vsnprintf(vstr->str + vstr->len, space_left, fmt, ap);
		va_end(ap);
		la_assert(ret >= 0);
		result_size = 1 + (size_t)ret;
		la_assert(result_size < space_left);
	}
end:
	vstr->len += result_size - 1;	// not including '\0'
	return;
}

void la_vstring_append_buffer(la_vstring * const vstr, void const * buffer, size_t len) {
	la_assert(vstr);
	if(buffer == NULL || len == 0) {
		return;
	}
	size_t space_left = la_vstring_space_left(vstr);
	if(len >= space_left) {
		la_vstring_grow(vstr, len);
	}
	la_assert(vstr->len + len <= LA_VSTR_SIZE_MAX);
	memcpy(vstr->str + vstr->len, buffer, len);
	vstr->len += len;
	vstr->str[vstr->len] = '\0';
	return;
}
