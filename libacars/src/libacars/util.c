/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdio.h>		// fprintf
#include <stdint.h>
#include <stdlib.h>		// calloc, realloc, free
#include <string.h>		// strerror, strlen
#include <errno.h>		// errno
#include <unistd.h>		// _exit
#include <libacars/macros.h>	// la_debug_print()
#include <libacars/util.h>

void *la_xcalloc(size_t nmemb, size_t size, const char *file, const int line, const char *func) {
	void *ptr = calloc(nmemb, size);
	if(ptr == NULL) {
		fprintf(stderr, "%s:%d: %s(): calloc(%zu, %zu) failed: %s\n",
			file, line, func, nmemb, size, strerror(errno));
		_exit(1);
	}
	return ptr;
}

void *la_xrealloc(void *ptr, size_t size, const char *file, const int line, const char *func) {
	ptr = realloc(ptr, size);
	if(ptr == NULL) {
		fprintf(stderr, "%s:%d: %s(): realloc(%zu) failed: %s\n",
			file, line, func, size, strerror(errno));
		_exit(1);
	}
	return ptr;
}

void *la_dict_search(const la_dict *list, int id) {
	if(list == NULL) return NULL;
	la_dict *ptr;
	for(ptr = (la_dict *)list; ; ptr++) {
		if(ptr->val == NULL) return NULL;
		if(ptr->id == id) return ptr->val;
	}
}

size_t la_slurp_hexstring(char* string, uint8_t **buf) {
	if(string == NULL)
		return 0;
	size_t slen = strlen(string);
	if(slen & 1)
		slen--;
	size_t dlen = slen / 2;
	if(dlen == 0)
		return 0;
	*buf = LA_XCALLOC(dlen, sizeof(uint8_t));

	for(size_t i = 0; i < slen; i++) {
		char c = string[i];
		int value = 0;
		if(c >= '0' && c <= '9') {
			value = (c - '0');
		} else if (c >= 'A' && c <= 'F') {
			value = (10 + (c - 'A'));
		} else if (c >= 'a' && c <= 'f') {
			 value = (10 + (c - 'a'));
		} else {
			la_debug_print("stopped at invalid char %u at pos %zu\n", c, i);
			return i/2;
		}
		(*buf)[(i/2)] |= value << (((i + 1) % 2) * 4);
	}
	return dlen;
}
