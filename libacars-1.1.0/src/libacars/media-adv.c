/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdbool.h>
#include <ctype.h>			// isupper(), isdigit()
#include <string.h>			// strchr(), strcpy(), strncpy(), strlen()
#include <libacars/libacars.h>		// la_proto_node, la_proto_tree_find_protocol()
#include <libacars/media-adv.h>		// la_arinc_msg, LA_ARINC_IMI_CNT
#include <libacars/macros.h>		// la_debug_print()
#include <libacars/vstring.h>		// la_vstring, la_vstring_append_sprintf(),
					// LA_ISPRINTF()
#include <libacars/util.h>		// LA_XCALLOC()

typedef struct {
	char const code;
	char const * const description;
} la_media_adv_link_type_map;

static la_media_adv_link_type_map const link_type_map[LA_MEDIA_ADV_LINK_TYPE_CNT] = {
	{ .code = 'V', .description = "VHF ACARS" },
	{ .code = 'S', .description = "Default SATCOM" },
	{ .code = 'H', .description = "HF" },
	{ .code = 'G', .description = "Global Star Satcom" },
	{ .code = 'C', .description = "ICO Satcom" },
	{ .code = '2', .description = "VDL2" },
	{ .code = 'X', .description = "Inmarsat Aero H/H+/I/L" },
	{ .code = 'I', .description = "Iridium Satcom" }
};

static char const *get_link_description(char code) {
	for(int k = 0; k < LA_MEDIA_ADV_LINK_TYPE_CNT; k++) {
		if(link_type_map[k].code == code) {
			return link_type_map[k].description;
		}
	}
	return NULL;
}

static bool is_numeric(char const *str, size_t len) {
	if(!str) return false;
	for(size_t i = 0; i < len; i++) {
		if(!isdigit(str[i]) || str[i] == '\0') {
			return false;
		}
	}
	return true;
}

static bool check_format(char const *txt) {
	bool valid = false;
	int payload_len = strlen(txt);
	if(payload_len >= 10) {
		valid = txt[0] == '0';
		valid &= txt[1] == 'E' || txt[1] == 'L';
		valid &= strchr("VSHGC2XI",txt[2]) != NULL;
		valid &= is_numeric(&txt[3], 6);
		int index = 9;
		while(txt[index] !='\0' && txt[index] != '/') {
			valid &= strchr("VSHGC2XI",txt[index++]) != NULL;
		}
	}
	return valid;
}

la_proto_node *la_media_adv_parse(char const *txt) {
	if(txt == NULL) {
		return NULL;
	}

	la_media_adv_msg *msg = LA_XCALLOC(1, sizeof(la_media_adv_msg));
	la_proto_node *node = NULL;
	la_proto_node *next_node = NULL;
	// default to error
	msg->err = true;

	int payload_len = strlen(txt);
	// Message size 0EV122234V
	if(check_format(txt)) {
		msg->err = false;
		// First is version
		msg->version[0] = txt[0];
		msg->version[1] = '\0';
		// link status Established or Lost
		msg->state[0] = txt[1];
		msg->state[1] = '\0';
		// link type
		msg->current_link[0] = txt[2];
		msg->current_link[1] = '\0';
		// time of state change
		strncpy(msg->hour, &txt[3], 2);
		if(atoi(msg->hour)>23) {
			msg->err = true;
		}
		strncpy(msg->minute, &txt[5], 2);
		if(atoi(msg->minute)>59) {
			msg->err = true;
		}
		strncpy(msg->second, &txt[7], 2);
		if(atoi(msg->second)>59) {
			msg->err = true;
		}
		// Available links are for 4 to symbol / if present
		char *end = strchr(txt, '/');
		// if there is no / only available links are present
		if(end == NULL)  {
			int index = 9;
			while(index < payload_len) {
				msg->available_links[index - 9] = txt[index];
				index++;
			}
			msg->available_links[index - 9] = '\0';
			msg->text[0] = '\0';
		} else {
			// Copy all link until / is found
			int index = 9;
			while(index < payload_len) {
				if(txt[index] != '/') {
					msg->available_links[index - 9] = txt[index];
				} else {
					break;
				}
				index++;
			}
			msg->available_links[index - 9] = '\0';
			// copy text
			strcpy(msg->text, end + 1);
		}
	}

	node = la_proto_node_new();
	node->data = msg;
	node->td = &la_DEF_media_adv_message;
	node->next = next_node;
	return node;
}

void la_media_adv_format_text(la_vstring * const vstr, void const * const data, int indent) {
	la_assert(vstr);
	la_assert(data);
	la_assert(indent >= 0);

	LA_CAST_PTR(msg, la_media_adv_msg *, data);

	if(msg->err == true) {
		LA_ISPRINTF(vstr, indent, "%s", "-- Unparseable Media Advisory message\n");
		return;
	}

	// Version
	LA_ISPRINTF(vstr, indent, "Media Advisory, version %s:\n", msg->version);
	indent++;

	// Prepare time
	LA_ISPRINTF(vstr, indent, "Link %s %s at %s:%s:%s UTC\n",
		get_link_description(msg->current_link[0]),
		(msg->state[0] == 'E') ? "established" : "lost",
		msg->hour, msg->minute, msg->second
	);

	// print all available links
	LA_ISPRINTF(vstr, indent, "%s", "Available links: ");
	int count = (int)strlen(msg->available_links);
	for(int i = 0; i < count; i++) {
		const char *link = get_link_description(msg->available_links[i]);
		if(i == count - 1) {
			la_vstring_append_sprintf(vstr, "%s\n", link);
		} else {
			la_vstring_append_sprintf(vstr, "%s, ", link);
		}
	}

	// print text if present
	if(strlen(msg->text)) {
		LA_ISPRINTF(vstr, indent, "Text: %s\n", msg->text);
	}
}

la_type_descriptor const la_DEF_media_adv_message = {
	.format_text = la_media_adv_format_text,
	.destroy = NULL
};

la_proto_node *la_proto_tree_find_media_adv(la_proto_node *root) {
	return la_proto_tree_find_protocol(root, &la_DEF_media_adv_message);
}
