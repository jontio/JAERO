/*
 *  cpdlc_get_position - an example program showing how to extract
 *  aircraft position information from downlink CPDLC message with
 *  a position report.
 *
 *  Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <libacars/asn1/FANSATCDownlinkMessage.h>
#include <libacars/asn1/FANSATCDownlinkMsgElementId.h>
#include <libacars/asn1/FANSATCDownlinkMsgElementIdSequence.h>
#include <libacars/asn1/FANSLatitudeLongitude.h>
#include <libacars/asn1/FANSPositionCurrent.h>
#include <libacars/asn1/FANSPositionReport.h>
#include <libacars/libacars.h>
#include <libacars/arinc.h>
#include <libacars/cpdlc.h>

void usage() {
	fprintf(stderr,
	"cpdlc_get_position - extracts position information from CPDLC Position Report\n"
	"(c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>\n\n"
	"Usage:\n\n"
	"To decode a single message from command line:\n\n"
	"\t./cpdlc_get_position <acars_message_text>\n\n"
	"Enclose ACARS message text in quotes if it contains spaces or other shell\n"
	"special shell characters, like '#'.\n\n"
	"Example: ./cpdlc_get_position '/SOUCAYA.AT1.HL8251243F880C3D903BB412903604FE326C2479F4A64F7F62528B1A9CF8382738186AC28B16668E013DF464D8A7F0'\n\n"
	"To decode multiple messages from a text file:\n\n"
	"1. Prepare a file with multiple messages, one per line."
	"   Example:\n\n"
	"/SOUCAYA.AT1.HL8251243F880C3D903BB412903604FE326C2479F4A64F7F62528B1A9CF8382738186AC28B16668E013DF464D8A7F0\n"
	"/MSTEC7X.AT1.VT-ANKA094D88C3D903BB465D0723053B2E5123CFA53279400014B0894A2C6A73CBD8F52447AF1244CB4C9B94600089D65C84314892694587510528B1A9CF41169D440C1AB36A08B42\n"
	"/MELCAYA.AT1.ZK-OKC253C21CC3D903BA178F96618F0B28024B83127CD7886A12E9D85266B927584A9169C1A8EEB2800EEA7\n\n"
	"2. Run cpdlc_get_position and pipe the the file contents on standard input:\n\n"
	"\t./cpdlc_get_position < messages.txt\n\n");
}

double parse_coordinate(long degrees, long *tenthsofminutes) {
	double result = (double)degrees;
	if(tenthsofminutes != NULL) {
		result += (double)(*tenthsofminutes) / 10.0f / 60.0f;
	}
	return result;
}

long parse_altitude(FANSAltitude_t altitude) {
	long result = -1;
	static const double meters2feet = 3.28084;

// Altitude may be expressed in various units. Some altitude types require
// multiplying by a constant to get the actual value (refer to the comments in
// src/libacars/asn1/fans-cpdlc.asn1 for details).
	switch(altitude.present) {
	case FANSAltitude_PR_altitudeQNH:
		result = altitude.choice.altitudeQNH * 10;
		break;
	case FANSAltitude_PR_altitudeQNHMeters:
		result = (long)round((double)altitude.choice.altitudeQNHMeters * meters2feet);
		break;
	case FANSAltitude_PR_altitudeQFE:
		result = altitude.choice.altitudeQFE * 10;
		break;
	case FANSAltitude_PR_altitudeQFEMeters:
		result = (long)round((double)altitude.choice.altitudeQFEMeters * meters2feet);
		break;
	case FANSAltitude_PR_altitudeGNSSFeet:
		result = altitude.choice.altitudeGNSSFeet;
		break;
	case FANSAltitude_PR_altitudeGNSSMeters:
		result = (long)round((double)altitude.choice.altitudeGNSSMeters * meters2feet);
		break;
	case FANSAltitude_PR_altitudeFlightLevel:
		result = altitude.choice.altitudeFlightLevel * 100;
		break;
	case FANSAltitude_PR_altitudeFlightLevelMetric:
		result = (long)round((double)altitude.choice.altitudeFlightLevelMetric * 10.0 * meters2feet);
		break;
	case FANSAltitude_PR_NOTHING:        /* No components present */
	default:
		break;
	}
	return result;
}

void extract_position(FANSPositionReport_t rpt) {
// FANSPositionCurrent_t can contain various types of position data (fix name, navaid, etc).
// We want only latitude/longitude coordinates.
	FANSPositionCurrent_t pos = rpt.positioncurrent;
	if(pos.present != FANSPosition_PR_latitudeLongitude) {
		printf("-- No latitude/longitude data present in this position report\n");
		return;
	}
	FANSLatitudeLongitude_t latlon = pos.choice.latitudeLongitude;
	double lat = parse_coordinate(latlon.latitude.latitudeDegrees, latlon.latitude.minutesLatLon);
	if(latlon.latitude.latitudeDirection == FANSLatitudeDirection_south) {
		lat = -lat;
	}
	double lon = parse_coordinate(latlon.longitude.longitudeDegrees, latlon.longitude.minutesLatLon);
	if(latlon.longitude.longitudeDirection == FANSLongitudeDirection_west) {
		lon = -lon;
	}
	long hours = rpt.timeatpositioncurrent.hours;
	long minutes = rpt.timeatpositioncurrent.minutes;
	long alt = parse_altitude(rpt.altitude);
	printf(" Latitude: %f\n Longitude: %f\n Altitude: %ld ft\n Time: %02ld:%02ld\n",
		lat, lon, alt, hours, minutes);
}

void parse(char *txt) {
// Parse the message and build the protocol tree
	la_proto_node *node = la_arinc_parse(txt, LA_MSG_DIR_AIR2GND);
	printf("%s\n", txt);
	if(node == NULL) {
		printf("-- Failed to decode message\n");
		return;
	}
// Walk the tree and find out if there is an CPDLC message in it.
// If there is, cpdlc_node will point at its node in the tree.
	la_proto_node *cpdlc_node = la_proto_tree_find_cpdlc(node);
	if(cpdlc_node == NULL) {
		printf("-- Not a CPDLC message\n");
		goto cleanup;
	}
	la_cpdlc_msg *msg = (la_cpdlc_msg *)cpdlc_node->data;
// Print a warning if the decoder has raised an error flag.
// The flag means that the message decoding has failed, at least partially.
// For example, it might have been an uplink message (while we expect
// a downlink) or it might have been truncated (decoded partially).
// We don't fail here, hoping for the second case - if basic report tag
// has been decoded correctly, we might get a good position.
	if(msg->err == true) {
		printf("-- Malformed CPDLC message (not a downlink?)\n");
		goto cleanup;
	}
// We have a good downlink CPDLC message.
// Find out if it contains a dM48PositionReport downlink message element.
// It may either appear in aTCuplinkmsgelementId (which contains the first
// message element) or in a aTCuplinkmsgelementid-seqOf sequence,
// which is optional and may contain up to four message elements.
	FANSATCDownlinkMessage_t *dm = (FANSATCDownlinkMessage_t *)msg->data;
	if(dm == NULL) {
		printf("-- Empty CPDLC message\n");
		goto cleanup;
	}
	if(dm->aTCDownlinkmsgelementid.present == FANSATCDownlinkMsgElementId_PR_dM48PositionReport) {
		extract_position(dm->aTCDownlinkmsgelementid.choice.dM48PositionReport);
		goto cleanup;
	}
	FANSATCDownlinkMsgElementIdSequence_t *dmeid_seq = dm->aTCdownlinkmsgelementid_seqOf;
	if(dmeid_seq != NULL) {
		for(int i = 0; i < dmeid_seq->list.count; i++) {
			FANSATCDownlinkMsgElementId_t *dmeid_ptr = (FANSATCDownlinkMsgElementId_t *)dmeid_seq->list.array[i];
			if(dmeid_ptr != NULL && dmeid_ptr->present == FANSATCDownlinkMsgElementId_PR_dM48PositionReport) {
				extract_position(dmeid_ptr->choice.dM48PositionReport);
				goto cleanup;
			}
		}
	}
	printf("-- Message does not contain CPDLC Position Report\n");

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
