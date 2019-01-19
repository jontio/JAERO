/*
 *  This file is a part of libacars
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */

#ifndef LA_ASN1_FORMAT_CPDLC_H
#define LA_ASN1_FORMAT_CPDLC_H 1
#include <libacars/asn1/asn_application.h>	// asn_TYPE_descriptor_t
#include <libacars/vstring.h>			// la_vstring

// asn1-format-cpdlc.c
void la_asn1_output_cpdlc_as_text(la_vstring *vstr, asn_TYPE_descriptor_t *td, const void *sptr, int indent);

#endif // !LA_ASN1_FORMAT_CPDLC_H
