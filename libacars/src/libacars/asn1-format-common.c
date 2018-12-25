/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 */

#include <libacars/asn1/asn_application.h>	// asn_TYPE_descriptor_t, asn_sprintf
#include <libacars/asn1/IA5String.h>		// IA5String_t
#include <libacars/asn1/INTEGER.h>		// asn_INTEGER_enum_map_t
#include <libacars/asn1/constr_CHOICE.h>	// _fetch_present_idx()
#include <libacars/asn1/asn_SET_OF.h>		// _A_CSET_FROM_VOID()
#include <libacars/asn1-util.h>			// LA_ASN1_FORMATTER_PROTOTYPE
#include <libacars/macros.h>			// LA_CAST_PTR
#include <libacars/util.h>			// la_dict_search
#include <libacars/vstring.h>			// la_vstring, la_vstring_append_sprintf(), LA_ISPRINTF

char const *la_value2enum(asn_TYPE_descriptor_t *td, long const value) {
	if(td == NULL) return NULL;
	asn_INTEGER_enum_map_t const *enum_map = INTEGER_map_value2enum(td->specifics, value);
	if(enum_map == NULL) return NULL;
	return enum_map->enum_name;
}

void la_format_INTEGER_with_unit(la_vstring *vstr, char const * const label, asn_TYPE_descriptor_t *td,
	void const *sptr, int indent, char const * const unit, double multiplier, int decimal_places) {
// -Wunused-parameter
	(void)td;
	LA_CAST_PTR(val, long *, sptr);
	LA_ISPRINTF(vstr, indent, "%s: %.*f%s\n", label, decimal_places, (double)(*val) * multiplier, unit);
}

void la_format_CHOICE(la_vstring *vstr, char const * const label, la_dict const * const choice_labels,
	asn1_output_fun_t cb, asn_TYPE_descriptor_t *td, void const *sptr, int indent) {

	asn_CHOICE_specifics_t *specs = (asn_CHOICE_specifics_t *)td->specifics;
	int present = _fetch_present_idx(sptr, specs->pres_offset, specs->pres_size);
	if(label != NULL) {
		LA_ISPRINTF(vstr, indent, "%s:\n", label);
		indent++;
	}
	if(choice_labels != NULL) {
		char *descr = la_dict_search(choice_labels, present);
		if(descr != NULL) {
			LA_ISPRINTF(vstr, indent, "%s\n", descr);
		} else {
			LA_ISPRINTF(vstr, indent, "<no description for CHOICE value %d>\n", present);
		}
		indent++;
	}
	if(present > 0 && present <= td->elements_count) {
		asn_TYPE_member_t *elm = &td->elements[present-1];
		void const *memb_ptr;

		if(elm->flags & ATF_POINTER) {
			memb_ptr = *(const void * const *)((const char *)sptr + elm->memb_offset);
			if(!memb_ptr) {
				LA_ISPRINTF(vstr, indent, "%s: <not present>\n", elm->name);
				return;
			}
		} else {
			memb_ptr = (const void *)((const char *)sptr + elm->memb_offset);
		}

		cb(vstr, elm->type, memb_ptr, indent);
	} else {
		LA_ISPRINTF(vstr, indent, "-- %s: value %d out of range\n", td->name, present);
	}
}

void la_format_SEQUENCE(la_vstring *vstr, char const * const label, asn1_output_fun_t cb,
	asn_TYPE_descriptor_t *td, void const *sptr, int indent) {
	if(label != NULL) {
		LA_ISPRINTF(vstr, indent, "%s:\n", label);
		indent++;
	}
	for(int edx = 0; edx < td->elements_count; edx++) {
		asn_TYPE_member_t *elm = &td->elements[edx];
		const void *memb_ptr;

		if(elm->flags & ATF_POINTER) {
			memb_ptr = *(const void * const *)((const char *)sptr + elm->memb_offset);
			if(!memb_ptr) {
				continue;
			}
		} else {
			memb_ptr = (const void *)((const char *)sptr + elm->memb_offset);
		}
		cb(vstr, elm->type, memb_ptr, indent);
	}
}

void la_format_SEQUENCE_OF(la_vstring *vstr, char const * const label, asn1_output_fun_t cb,
	asn_TYPE_descriptor_t *td, void const *sptr, int indent) {
	if(label != NULL) {
		LA_ISPRINTF(vstr, indent, "%s:\n", label);
		indent++;
	}
	asn_TYPE_member_t *elm = td->elements;
	const asn_anonymous_set_ *list = _A_CSET_FROM_VOID(sptr);
	for(int i = 0; i < list->count; i++) {
		const void *memb_ptr = list->array[i];
		if(memb_ptr == NULL) {
			continue;
		}
		cb(vstr, elm->type, memb_ptr, indent);
	}
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_any) {
	if(label != NULL) {
		LA_ISPRINTF(vstr, indent, "%s: ", label);
	} else {
		LA_ISPRINTF(vstr, indent, "%s", "");
	}
	asn_sprintf(vstr, td, sptr, 1);
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_IA5String) {
	LA_CAST_PTR(ia5str, IA5String_t *, sptr);
// replace nulls with periods for printf() to work correctly
	char *buf = (char *)ia5str->buf;
	for(int i = 0; i < ia5str->size; i++) {
		if(buf[i] == '\0') buf[i] = '.';
	}
	if(label != NULL) {
		LA_ISPRINTF(vstr, indent, "%s: ", label);
	} else {
		LA_ISPRINTF(vstr, indent, "%s", "");
	}
	asn_sprintf(vstr, td, sptr, 1);
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_NULL) {
// -Wunused-parameter
	(void)vstr;
	(void)label;
	(void)td;
	(void)sptr;
	(void)indent;
	// NOOP
}

LA_ASN1_FORMATTER_PROTOTYPE(la_asn1_format_text_ENUM) {
	long const value = *(long const *)sptr;
	char const *s = la_value2enum(td, value);
	if(s != NULL) {
		LA_ISPRINTF(vstr, indent, "%s: %s\n", label, s);
	} else {
		LA_ISPRINTF(vstr, indent, "%s: %ld\n", label, value);
	}
}
