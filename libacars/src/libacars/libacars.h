/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_LIBACARS_H
#define LA_LIBACARS_H 1
#include <stdbool.h>
#include <libacars/version.h>
#include <libacars/vstring.h>		// la_vstring

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	LA_MSG_DIR_UNKNOWN,
	LA_MSG_DIR_GND2AIR,
	LA_MSG_DIR_AIR2GND
} la_msg_dir;

typedef void (la_print_type_f)(la_vstring * const vstr, void const * const data, int indent);
typedef void (la_destroy_type_f)(void *data);

typedef struct {
	bool dump_asn1;
// padding for ABI compatibility
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_config_struct;

typedef struct {
	la_print_type_f *format_text;
	la_destroy_type_f *destroy;
// padding for ABI compatibility
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
	void (*reserved4)(void);
	void (*reserved5)(void);
	void (*reserved6)(void);
	void (*reserved7)(void);
	void (*reserved8)(void);
	void (*reserved9)(void);
} la_type_descriptor;

typedef struct la_proto_node la_proto_node;

struct la_proto_node {
	la_type_descriptor const *td;
	void *data;
	la_proto_node *next;
// padding for ABI compatibility
	void (*reserved0)(void);
	void (*reserved1)(void);
	void (*reserved2)(void);
	void (*reserved3)(void);
};

// libacars.c
extern la_config_struct la_config;
la_proto_node *la_proto_node_new();
la_vstring *la_proto_tree_format_text(la_vstring *vstr, la_proto_node const * const root);
void la_proto_tree_destroy(la_proto_node *root);
la_proto_node *la_proto_tree_find_protocol(la_proto_node *root, la_type_descriptor const * const td);

#ifdef __cplusplus
}
#endif

#endif // !LA_LIBACARS_H
