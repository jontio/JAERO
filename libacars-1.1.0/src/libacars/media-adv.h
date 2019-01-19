/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_MEDIA_ADV_H
#define LA_MEDIA_ADV_H 1

#include <stdint.h>
#include <libacars/libacars.h>		// la_type_descriptor, la_proto_node
#include <libacars/vstring.h>		// la_vstring

#ifdef __cplusplus
extern "C" {
#endif

#define LA_MEDIA_ADV_LINK_TYPE_CNT 8

typedef struct {
	char version[2];
	char state[2];
	char current_link[2];
	char hour[3];
	char minute[3];
	char second[3];
	char available_links[10];
	char text[255];
	bool err;
} la_media_adv_msg;

la_proto_node *la_media_adv_parse(char const *txt);
void la_media_adv_format_text(la_vstring * const vstr, void const * const data, int indent);
extern la_type_descriptor const la_DEF_media_adv_message;
la_proto_node *la_proto_tree_find_media_adv(la_proto_node *root);

#ifdef __cplusplus
}
#endif

#endif // !LA_MEDIA_ADV_H
