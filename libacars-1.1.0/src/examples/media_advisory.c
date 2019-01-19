/*
 *  media_advisory - an example program for decoding
 *  ACARS Media Advisory messages.
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <libacars/libacars.h>
#include <libacars/media-adv.h>

void usage() {
	fprintf(stderr,
	"media_advisory - extracts media advisory\n"
	"(c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>\n\n"
	"Usage:\n\n"
	"To decode a single message from command line:\n\n"
	"\t./media_advisory <acars_message_text>\n\n"
	"Enclose ACARS message text in quotes if it contains spaces or other shell\n"
	"special shell characters, like '#'.\n\n"
	"Example: ./media_advisory '0EV123324HS2/Test text'\n\n"
	"To decode multiple messages from a text file:\n\n"
	"1. Prepare a file with multiple messages, one per line. Each message\n"
	"   may optionally be prepended with an arbitrary prefix and a space,\n"
	"   but make sure there are no trailing spaces. Example:\n\n"
	"0EV134509V\n"
	"0L2034509HS\n"
	"any string 0EH104509H/\n"
	"05/01/2019 00:04:40 SQ0321 9V-SKR or something 0EH104509HV/\n\n"
	"2. Run media_advisory and pipe the the file contents on standard input:\n\n"
	"\t./media_advisory < messages.txt\n\n");
}

void parse(char *txt) {
	char *ptr = txt;
	char *end = strchr(txt, '\n');
	if(end) {
		*end = '\0';
	}
// Skip arbitrary string at the beginning.
// Warning: this won't work correctly if the message text itself contains a
// free text field (after '/' character) containing a space. However, free text
// field is not used in practice, so it's not a big deal.
	char *start = strrchr(txt, ' ');
	if(start) {
		ptr = start + 1;
	}
	// Parse the message and build the protocol tree
	la_proto_node *node = la_media_adv_parse(ptr);
	printf("%s\n", txt);
	if(node != NULL) {
		la_vstring *vstr = la_proto_tree_format_text(NULL, node);
		printf("%s\n", vstr->str);
		la_vstring_destroy(vstr, true);
	}
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
