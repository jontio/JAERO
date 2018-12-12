/*
 *  decode_arinc - an example of ARINC 622 ATS message decoder
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>		// getenv()
#include <string.h>
#include <libacars/libacars.h>
#include <libacars/arinc.h>
#include <libacars/vstring.h>

void usage() {
	fprintf(stderr,
	"decode_arinc - an example of ARINC 622 ATS message decoder (CPDLC, ADS-C)\n"
	"(c) 2018 Tomasz Lemiech <szpajder@gmail.com>\n\n"
	"Usage:\n\n"
	"To decode a single message from command line:\n\n"
	"\t./decode_arinc <direction> <acars_message_text>\n\n"
	"where <direction> is one of:\n"
	"\tu - means \"uplink\" (ground-to-air message)\n"
	"\td - means \"downlink\" (air-to-ground message)\n\n"
	"Enclose ACARS message text in quotes if it contains spaces or other shell\n"
	"special shell characters, like '#'.\n\n"
	"Example: ./decode_arinc d '/BOMASAI.ADS.VT-ANB072501A070A988CA73248F0E5DC10200000F5EE1ABC000102B885E0A19F5'\n\n"
	"To decode multiple messages from a text file:\n\n"
	"1. Prepare a file with multiple messages, one per line. Precede each line\n"
	"   with 'u' or 'd' (to indicate message direction) and a space. Direction\n"
	"   indicator must appear as a first character on the line (no preceding\n"
	"   spaces please). Example:\n\n"
	"u /AKLCDYA.AT1.9M-MTB215B659D84995674293583561CB9906744E9AF40F9EB\n"
	"d /CTUE1YA.ADS.HB-JNB1424AB686D9308CA2EBA1D0D24A2C06C1B48CA004A248050667908CA004BF6\n"
	"d /MSTEC7X.AT1.VT-ANE21409DCC3DD03BB52350490502B2E5129D5A15692BA009A08892E7CC831E210A4C06EEBC28B1662BC02360165C80E1F7\n"
	"u - #MD/AA ATLTWXA.CR1.N856DN203A3AA8E5C1A9323EDD\n\n"
	"2. Run decode_arinc and pipe the the file contents on standard input:\n\n"
	"\t./decode_arinc < messages.txt\n\n");
}

void parse(char *txt, la_msg_dir msg_dir) {
	la_proto_node *node = la_arinc_parse(txt, msg_dir);
	printf("%s\n", txt);
	if(node != NULL) {
		la_vstring *vstr = la_proto_tree_format_text(NULL, node);
		printf("%s\n", vstr->str);
		la_vstring_destroy(vstr, true);
	}
	la_proto_tree_destroy(node);
}

int main(int argc, char **argv) {
	la_msg_dir msg_dir = LA_MSG_DIR_UNKNOWN;
	char *dump_asn1 = getenv("ENABLE_ASN1_DUMPS");
	if(dump_asn1 != NULL && !strcmp(dump_asn1, "1")) {
		la_config.dump_asn1 = true;
	}
	if(argc > 1 && !strcmp(argv[1], "-h")) {
		usage();
		exit(0);
	} else if(argc < 2) {
		fprintf(stderr,
			"No command line options found - reading messages from standard input.\n"
			"Use '-h' option for help.\n"
		);

		char buf[1024];
		for(;;) {
			memset(buf, 0, sizeof(buf));
			msg_dir = LA_MSG_DIR_UNKNOWN;
			if(fgets(buf, sizeof(buf), stdin) == NULL)
				break;
			char *end = strchr(buf, '\n');
			if(end)
				*end = '\0';
			if(strlen(buf) < 3 || (buf[0] != 'u' && buf[0] != 'd') || buf[1] != ' ') {
				fprintf(stderr, "Garbled input: expecting 'u|d acars_message_text'\n");
				continue;
			}
			if(buf[0] == 'u')
				msg_dir = LA_MSG_DIR_GND2AIR;
			else if(buf[0] == 'd')
				msg_dir = LA_MSG_DIR_AIR2GND;
			parse(buf, msg_dir);
		}
	} else if(argc == 3) {
		if(argv[1][0] == 'u')
			msg_dir = LA_MSG_DIR_GND2AIR;
		else if(argv[1][0] == 'd')
			msg_dir = LA_MSG_DIR_AIR2GND;
		else {
			fprintf(stderr, "Invalid command line options\n\n");
			usage();
			exit(1);
		}
		parse(argv[2], msg_dir);
	} else {
		fprintf(stderr, "Invalid command line options\n\n");
		usage();
		exit(1);
	}
}
