/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_ACARS_H
#define LA_ACARS_H 1
#include <stdint.h>
#include <stdbool.h>
#include <libacars/libacars.h>			// la_proto_node, la_type_descriptor
#include <libacars/vstring.h>			// la_vstring

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	bool crc_ok;
	bool err;
	char mode;
	char reg[8];
	char ack;
	char label[3];
	char block_id;
	char no[5];
	char flight_id[7];
	char *txt;
} la_acars_msg;

// acars.c
extern la_type_descriptor const la_DEF_acars_message;
la_proto_node *la_acars_decode_apps(char const * const label,
	char const * const txt, la_msg_dir const msg_dir);
la_proto_node *la_acars_parse(uint8_t *buf, int len, la_msg_dir msg_dir);
void la_acars_format_text(la_vstring *vstr, void const * const data, int indent);
la_proto_node *la_proto_tree_find_acars(la_proto_node *root);
#ifdef __cplusplus
}
#endif
#endif // !LA_ACARS_H
