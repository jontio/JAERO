/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdbool.h>
#include <libacars/macros.h>		// la_assert
#include <libacars/libacars.h>		// la_proto_node
#include <libacars/vstring.h>
#include <libacars/util.h>		// LA_XCALLOC, LA_XFREE

la_config_struct la_config = {
	.dump_asn1 = false
};

static void la_proto_node_format_text(la_vstring * const vstr, la_proto_node const * const node, int indent) {
	la_assert(indent >= 0);
	if(node->data != NULL) {
		la_assert(node->td);
		node->td->format_text(vstr, node->data, indent);
	}
	if(node->next != NULL) {
		la_proto_node_format_text(vstr, node->next, indent+1);
	}
}

la_proto_node *la_proto_node_new() {
	la_proto_node *node = LA_XCALLOC(1, sizeof(la_proto_node));
	return node;
}

la_vstring *la_proto_tree_format_text(la_vstring *vstr, la_proto_node const * const root) {
	la_assert(root);

	if(vstr == NULL) {
		vstr = la_vstring_new();
	}
	la_proto_node_format_text(vstr, root, 0);
	return vstr;
}

void la_proto_tree_destroy(la_proto_node *root) {
	if(root == NULL) {
		return;
	}
	if(root->next != NULL) {
		la_proto_tree_destroy(root->next);
	}
	if(root->td != NULL && root->td->destroy != NULL) {
		root->td->destroy(root->data);
	} else {
		LA_XFREE(root->data);
	}
	LA_XFREE(root);
}

la_proto_node *la_proto_tree_find_protocol(la_proto_node *root, la_type_descriptor const * const td) {
	while(root != NULL) {
		if(root->td == td) {
			return root;
		}
		root = root->next;
	}
	return NULL;
}
