/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <stdint.h>
#include <search.h>				// lfind()
#include <libacars/asn1/asn_application.h>	// asn_TYPE_descriptor_t
#include <libacars/asn1-util.h>			// la_asn_formatter
#include <libacars/macros.h>			// LA_CAST_PTR, LA_ISPRINTF, la_debug_print()
#include <libacars/vstring.h>			// la_vstring

static int la_compare_fmtr(const void *k, const void *m) {
	LA_CAST_PTR(memb, la_asn_formatter *, m);
	return(k == memb->type ? 0 : 1);
}

int la_asn1_decode_as(asn_TYPE_descriptor_t *td, void **struct_ptr, uint8_t *buf, int size) {
	asn_dec_rval_t rval;
	rval = uper_decode_complete(0, td, struct_ptr, buf, size);
	if(rval.code != RC_OK) {
		la_debug_print("uper_decode_complete failed: %d\n", rval.code);
		return -1;
	}
	if(rval.consumed < (size_t)size) {
		la_debug_print("uper_decode_complete left %zd unparsed octets\n", (size_t)size - rval.consumed);
		return (int)((size_t)size - rval.consumed);
	}
#ifdef DEBUG
	asn_fprint(stderr, td, *struct_ptr, 1);
#endif
	return 0;
}

void la_asn1_output(la_vstring *vstr, la_asn_formatter const * const asn1_formatter_table,
	size_t asn1_formatter_table_len, asn_TYPE_descriptor_t *td, const void *sptr, int indent) {
	if(td == NULL || sptr == NULL) return;
	la_asn_formatter *formatter = lfind(td, asn1_formatter_table, &asn1_formatter_table_len,
		sizeof(la_asn_formatter), &la_compare_fmtr);
	if(formatter != NULL) {
		formatter->format(vstr, formatter->label, td, sptr, indent);
	} else {
		LA_ISPRINTF(vstr, indent, "-- Formatter for type %s not found, ASN.1 dump follows:\n", td->name);
		if(indent > 0) {
			LA_ISPRINTF(vstr, indent * 4, "%s", "");	// asn_fprint does not indent the first line
		}
		asn_sprintf(vstr, td, sptr, indent+1);
		LA_ISPRINTF(vstr, indent, "%s", "-- ASN.1 dump end\n");
	}
}
