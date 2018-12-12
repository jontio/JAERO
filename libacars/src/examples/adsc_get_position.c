/*
 *  adsc_get_position - an example program showing how to extract
 *  aircraft position information from downlink ADS-C messages.
 *
 *  Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libacars/libacars.h>
#include <libacars/arinc.h>
#include <libacars/adsc.h>
#include <libacars/list.h>

void usage() {
	fprintf(stderr,
	"adsc_get_position - extracts position information from ADS-C Basic Report\n"
	"(c) 2018 Tomasz Lemiech <szpajder@gmail.com>\n\n"
	"Usage:\n\n"
	"To decode a single message from command line:\n\n"
	"\t./adsc_get_position <acars_message_text>\n\n"
	"Enclose ACARS message text in quotes if it contains spaces or other shell\n"
	"special shell characters, like '#'.\n\n"
	"Example: ./adsc_get_position '/BOMASAI.ADS.VT-ANB072501A070A988CA73248F0E5DC10200000F5EE1ABC000102B885E0A19F5'\n\n"
	"To decode multiple messages from a text file:\n\n"
	"1. Prepare a file with multiple messages, one per line."
	"   Example:\n\n"
	"/AUHASMO.ADS.A6-PFE0724D9586A36C92B2DCF1F0E74A8E4807C0F7219AF407C10422E9E08A1C4\n"
	"/CTUE1YA.ADS.HB-JNB1424AB686D9308CA2EBA1D0D24A2C06C1B48CA004A248050667908CA004BF6\n"
	"/YQXE2YA.ADS.SP-LRH1424FD087806C0B527769F0D2500B877ED00B5401E2516707755C01340B768\n\n"
	"2. Run adsc_get_position and pipe the the file contents on standard input:\n\n"
	"\t./adsc_get_position < messages.txt\n\n");
}

void parse(char *txt) {
// Parse the message and build the protocol tree
	la_proto_node *node = la_arinc_parse(txt, LA_MSG_DIR_AIR2GND);
	printf("%s\n", txt);
	if(node == NULL) {
		printf("-- Failed to decode message\n");
		return;
	}
// Walk the tree and find out if there is an ADS-C message in it.
// If there is, adsc_node will point at its node in the tree.
	la_proto_node *adsc_node = la_proto_tree_find_adsc(node);
	if(adsc_node == NULL) {
		printf("-- Not an ADS-C message\n");
		goto cleanup;
	}
	la_adsc_msg_t *msg = (la_adsc_msg_t *)adsc_node->data;
// Print a warning if the decoder has raised an error flag.
// The flag means that the message decoding has failed, at least partially.
// For example, it might have been an uplink message (while we expect
// a downlink) or it might have been truncated (decoded partially).
// We don't fail here, hoping for the second case - if basic report tag
// has been decoded correctly, we might get a good position.
	if(msg->err == true) {
		printf("-- Malformed ADS-C message\n");
	}
	la_list *l = msg->tag_list;
	la_adsc_basic_report_t *rpt = NULL;
// We have a good ADS-C message.
// Now walk the list of tags and find out if it contains a Basic Report tag
// (this is where coordinates are found).
	while(l != NULL) {
		la_adsc_tag_t *tag_struct = (la_adsc_tag_t *)l->data;
		uint8_t t = tag_struct->tag;
// These tags contain a Basic Report
		if(t == 7 || t == 9 || t == 10 || t == 18 || t == 19 || t == 20) {
			rpt = (la_adsc_basic_report_t *)tag_struct->data;
			break;
		}
		l = la_list_next(l);
	}
	if(rpt == NULL) {
		printf("-- Message does not contain an ADS-C Basic Report\n");
		goto cleanup;
	}
// We have found a Basic Report - print some fields from it.
	printf(" Latitude: %f\n Longitude: %f\n Altitude: %d ft\n Timestamp: %f seconds past hour\n",
		rpt->lat, rpt->lon, rpt->alt, rpt->timestamp);
cleanup:
	la_proto_tree_destroy(node);
}

int main(int argc, char **argv) {
	if(argc > 1 && !strcmp(argv[1], "-h")) {
		usage();
		return 0;
	} else if(argc < 2) {
		fprintf(stderr,
			"No command line options found - reading messages from standard input.\n"
			"Use '-h' option for help.\n"
		);

		char buf[1024];
		for(;;) {
			memset(buf, 0, sizeof(buf));
			if(fgets(buf, sizeof(buf), stdin) == NULL)
				break;
			char *end = strchr(buf, '\n');
			if(end)
				*end = '\0';
			parse(buf);
		}
	} else if(argc == 2) {
		parse(argv[1]);
	} else {
		fprintf(stderr, "Invalid command line options\n\n");
		usage();
		return 1;
	}
}
