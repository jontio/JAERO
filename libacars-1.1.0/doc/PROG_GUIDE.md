# libacars Programmer's Guide
Copyright (c) 2018-2019 Tomasz Lemiech <szpajder@gmail.com>

## Introduction

ACARS (an abbreviation of Aircraft Communications Addressing and Reporting
System) is a digital datalink system for transmission of short messages between
aircraft and ground stations. Messages may be carried over several media types,
including:

- VHF airband radio (aka "Plain old ACARS")
- VHF Data Link Mode 2 (VDL2)
- High Frequency Data Link (HFDL)
- Satellite

These media types vary in bit rate, modulation, channel coding and framing, so
there is no one-size-fits-all hardware+software setup which could receive,
demodulate and decode all of them. However once the data bytes comprising
the ACARS message have been recovered, further processing is the same
regardless of used medium.

### What libacars does

The purpose of libacars library is to provide a toolkit for ACARS message
processing and decoding. Developers of ACARS decoding applications are expected
to provide at least the RF and DSP part (demodulation, channel decoding, link
layer framing decapsulation). libacars will do the rest - check message CRC,
decode all fields and make their values accessible for possible further
processing.

Many ACARS messages contain human readable text or contain simple structured
data (alphanumeric fields with or without delimiter characters). Once the
message text has been recovered, there is nothing left to be done with it,
except of writing the message to a log file or screen. However ACARS messages
can also contain binary encoded data. A common example are air traffic services
applications, like ADS-C or CPDLC. libacars provides decoders for these kinds of
messages. The API allows access to all message fields and their values.
Pretty-printing routine for all supported payload types are provided as well.

### What libacars does not do

It does not interface with the radio (neither via SDR API nor audio through sound
card or virtual audio cable). It's the programmer's job to provide this part.

## General API concepts

The API revolves around a concept named protocol tree (aka proto_tree). It is a
data structure used to represent a multi-layered ACARS message. Let's have a
look at the following example:

```
ACARS:
 Reg: .VQ-BPJ Flight: SU2549
 Mode: 2 Label: B6 Blk id: 8 Ack: ! Msg no.: J61A
 Message:
  /LPAFAYA.ADS.VQ-BPJ1423CCA85D2D090886301D0D24C7D0704309088442255CC87CE2C90880DF97
 ADS-C message:
  Waypoint change event:
   Lat: 50.3429604
   Lon: 16.3785553
   Alt: 37000 ft
   Time: 396.000 sec past hour (:06:36.000)
   Position accuracy: <0.25 nm
   NAV unit redundancy: OK
   TCAS: OK
  Predicted route:
   Next waypoint:
    Lat: 51.7226028
    Lon: 19.7335052
    Alt: 37000 ft
    ETA: 1090 sec
   Next+1 waypoint:
    Lat: 52.5409126
    Lon: 21.9525719
    Alt: 37000 ft
```

One may say that it's just an ACARS message, but it would be an
oversimplification. It's actually three protocol data units (PDUs) encapsulated
into one another:

- The first (outer) layer is an ACARS message. It has a header with
  well-known fields, like aircraft registration number, flight number, mode,
  label and so forth. After the header comes the actual message text, starting
  with "/LPAFAYA...".
- The message text itself is a next layer of encapsulation - an ARINC-622 Air
  Traffic Services (ATS) application. It consists of five fields: the ground
  station address (LPAFAYA), the Imbedded Message Identifier (IMI) = "ADS", the
  air station address (VQ-BPJ), the hex-encoded binary part and the CRC (last four
  hex digits - "DF97")
- The hex-encoded binary part is a third layer - an Automatic Dependent
  Surveillance-Contract message containing two message groups - a waypoint
  change event group and predicted route group.

This message can be represented in a hierarchical manner:

```
ACARS
|
\-> ARINC-622
    |
    \-> ADS-C
```

This is how the message is represented in memory after being decoded by
libacars. Further down in this guide the whole structure will be called
"protocol tree", while the single tree level will be called "protocol node".
For each supported protocol or application libacars provides a function which
performs the decoding process and a function which serializes the decoded message
into human-readable text.

It is not required to start decoding or serialization at the top level of the tree.
If your program already handles ACARS message dissection, you may keep this part
unchanged and directly use ARINC-622 application decoder from libacars. If in
turn you have your own ADS-C decoder implementation in place, you may want to
use libacars only to decode CPDLC. This is possible as well. Same thing for
serialization - you may print the whole protocol tree at once with a single
function call or start printing at an intermediate protocol node of your choice.

## Library usage guidelines

### Compiler and linker flags

The preferred way to determine `CFLAGS` and `LDLIBS` required to compile your
program with libacars is to use `pkg-config`. libacars installs a file
named `libacars.pc` which contains include paths and library paths, so don't
hardcode them in your Makefiles. Instead just do:

```
$ pkg-config --cflags libacars
-I/usr/local/include
$ pkg-config --libs libacars
-L/usr/local/lib -lacars
```

to get the correct values.

If `pkg-config` fails with this error message:

```
Package libacars was not found in the pkg-config search path.
Perhaps you should add the directory containing `libacars.pc'
to the PKG_CONFIG_PATH environment variable
No package 'libacars' found
```

just do what it says. Assuming that you've installed libacars in the default
location (which is `/usr/local` on Unix), the `libacars.pc` file has landed in
the directory `/usr/local/lib/pkgconfig`. In this case do the following:

```
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/usr/local/lib/pkconfig
```

and it should do its job. You may want to set the `PKG_CONFIG_PATH` variable in
a persistent manner (ie. by adding the export line to your `~/.profile` or
`/etc/profile`).

### Debugging output

If you suspect a bug in the library or you are just struggling to get your
program working with libacars, it might be useful to enable debugging output.
The library supports two build types - "Debug" (with debugging output enabled)
and "Release" (debugging output disabled, which is the default). To switch the
build type to "Debug", go to the `build` subdirectory where you've build
libacars and do the following:

```
cmake -DCMAKE_BUILD_TYPE=Debug ../
make
sudo make install
```

Additionally, ASN.1 decoders (which are used to decode CPDLC) can produce their
own debugging messages. To enable ASN.1 debugging, enable "Debug" build and
additionally enable the `EMIT_ASN_DEBUG` flag:


```
cmake -DCMAKE_BUILD_TYPE=Debug -DEMIT_ASN_DEBUG=ON ../
make
sudo make install
```

Debug messages will be printed to stderr.

## How to decode anything

### Example 1: Decoding simple ACARS message

Let's assume that your program has successfully received, demodulated,
descrambled and deinterleaved a series of bytes which most probably is an ACARS
message. You have raw bytes stores in a buffer and want libacars to do the rest
of the work. Here is a simple program code:


```C
#include <stdint.h>		// uint8_t
#include <stdbool.h>		// bool
#include <stdio.h>		// printf(3)
#include <string.h>		// strlen(3)
#include <libacars/libacars.h>	// la_proto_node, la_proto_tree_destroy(),
				// la_proto_tree_format_text()
#include <libacars/acars.h>	// la_acars_parse()
#include <libacars/vstring.h>	// la_vstring, la_vstring_destroy()

int main() {
// Message buffer (raw bytes)
	uint8_t bytebuf[] =
	"\x01\x32\xae\xd3\xd0\xad\x4c\xc4\x45\x15\x32\xb3\xb3\x02\xcd"
	"\xb0\xb9\xc1\x4c\x4f\xb0\x32\xc4\xcd\x4f\xce\xce\xb0\x31\x4c"
	"\x4f\xb0\x32\xc4\xcd\x2f\x2a\x2a\x32\xb9\x32\xb0\x34\x31\x45"
	"\x4c\x4c\x58\x45\xd0\x57\xc1\x32\xb0\x34\x31\xb0\xb0\x32\x38"
	"\x83\xdf\xcb\x7f";
// Try to parse the buffer as an ACARS message
	la_proto_node *node = la_acars_parse(bytebuf+1, strlen(bytebuf)-1,
		LA_MSG_DIR_AIR2GND);
	if(node != NULL) {
// Format the result as a human-readable text
		la_vstring *vstr = la_proto_tree_format_text(NULL, node);
// Print the result
		printf("Decoded message:\n%s\n", vstr->str);
// Free memory used by variable string
		la_vstring_destroy(vstr, true);
	} else {
		printf("Sorry, decoder has failed\n");
	}
// Free memory used by the entire protocol tree
	la_proto_tree_destroy(node);
}
```

Save the code in a file named `deacars.c` and compile:

```
gcc `pkg-config --cflags libacars` `pkg-config --libs libacars` -o deacars deacars.c
```

Run it:

```
$ ./deacars
Decoded message:
ACARS:
 Reg: .SP-LDE Flight: LO02DM
 Mode: 2 Label: 23 Blk id: 3 Ack: ! Msg no.: M09A
 Message:
  ONN01LO02DM/**292041ELLXEPWA20410028
```

Success! You have decoded your first ACARS message using libacars. Now some
explanation on how it works:

- `bytebuf[]` contains raw message bytes. It starts with a SOH byte (`'\x01'`),
  and ends with DEL byte (`'\x7f'`). The fourth byte counting from the end is the
  ETX byte (`'\x83'` or actually `'\x03'`, because the most significant bit is
  a parity bit). Two bytes following ETX are CRC.

- `la_acars_parse()` is an ACARS message parser. It is declared in
  `<libacars/acars.h>` with a following prototype:

```C
la_proto_node *la_acars_parse(uint8_t *buf, int len, la_msg_dir msg_dir);
```

- The first argument is the pointer to the byte buffer. Note: `la_acars_parse()`
  wants the buffer to start with a first byte **after** the SOH byte, hence the
  argument in the example is `bytebuf + 1` and not just `bytebuf`.

- The second argument is the buffer length. Again - we skipped SOH, so we
  subtract 1 from the total buffer length.

- Third argument is the message direction. Decoders of bit-oriented ATS
  applications need this information to decode application layer data correctly
  and unambiguously. A value of `LA_MSG_DIR_AIR2GND` indicates that the message
  has been transmitted by an aircraft while `LA_MSG_DIR_GND2AIR` indicates
  transmission from ground station.  For most media types this information can
  be read from lower layer headers. The only exception for this is Plain Old ACARS
  on VHF, where the direction can be determined only from the value of block ID
  field, which means the ACARS header has to be decoded first. For this reason,
  there is a third value `LA_MSG_DIR_UNKNOWN`. It tells `la_acars_parse()` to
  determine the direction automatically.

- `la_acars_parse()` (and other protocol parsers too) returns a pointer to a
  `la_proto_node` structure which is the root of the decoded protocol tree. In
  the above case the message text does not contain data of any application
  supported by libacars, so the whole tree consists of just a single
  `la_proto_node` structure. But the protocol tree is actually a single-linked
  list, so in general it may contain more nodes linked together in a structure
  depicted above in the "General API concepts" section.

- No matter if decoding succeeded or failed, `la_acars_parse()` returns a
  non-NULL pointer.

- `la_proto_tree_format_text()` is prototyped as follows:

```C
la_vstring *la_proto_tree_format_text(la_vstring *vstr, la_proto_node const * const root);
```

- It takes a pointer named `root` (which must be non-NULL!) to the root of a
  protocol tree and produces human-readable output of the whole tree.  The
  result is stored in a structure of type `la_vstring`. If the result shall be
  appended to an existing string, the pointer to it shall be passed as the first
  argument.  Otherwise it shall be set to NULL, which causes a new `la_vstring` to
  be allocated automatically.

- `la_vstring` implements a variable-length string. Its field named `char *str`
  contains the actual string, which can be printf()ed, strdup()ed, used and
  abused in the same way as any other character buffer. `la_vstring` should be
  deallocated after use with `la_vstring_destroy()`. The second argument to this
  function indicates whether the char buffer `str` should be freed as well (true)
  or preserved (false).

- It is safe to invoke `la_proto_tree_format_text()` on a protocol node of a
  message which failed to decode. The result will then contain an error message.

### Example 2: Decoding ACARS applications

The workflow described in the first example is appropriate when writing a new
ACARS decoding program from scratch. But if you already have a working ACARS
decoder, most probably you only want to add some new decoding capabilities to
it. libacars does not force you to ditch your current ACARS message processing
code.

Suppose that you have parsed the raw byte buffer with your own decoder and now
you have some fields in char buffers - label, message text, possibly others. You
want libacars to decode these fields. How to achieve this?

```C
#include <stdbool.h>		// bool
#include <stdio.h>		// printf(3)
#include <libacars/libacars.h>	// la_proto_node, la_proto_tree_destroy(),
				// la_proto_tree_format_text()
#include <libacars/acars.h>	// la_acars_decode_apps()
#include <libacars/vstring.h>	// la_vstring, la_vstring_destroy()

int main() {
	char *label = "H1";
	char *message = "#M1B/B6 LHWE1YA.ADS.N572UP07263B5872A048C9F21C1F0E5B88D700000239";
	la_msg_dir direction = LA_MSG_DIR_AIR2GND;

// Check if this is a supported ACARS application. If it is, decode it.
	la_proto_node *node = la_acars_decode_apps(label, message, direction);
	if(node != NULL) {
		la_vstring *vstr = la_proto_tree_format_text(NULL, node);
		printf("Decoded message:\n%s\n", vstr->str);
		la_vstring_destroy(vstr, true);
	} else {
		printf("No supported ACARS application found\n");
	}
	la_proto_tree_destroy(node);
}
```

Result:

```
$ ./decode_apps
Decoded message:
ADS-C message:
 Basic report:
  Lat: 53.7634850
  Lon: 20.1490974
  Alt: 35996 ft
  Time: 3207.000 sec past hour (:53:27.000)
  Position accuracy: <0.05 nm
  NAV unit redundancy: OK
  TCAS: OK
 Earth reference data:
  True track: 257.4 deg
  Ground speed: 430.0 kt
  Vertical speed: 0 ft/min
```

`la_acars_decode_apps()` tries to determine the application using message label
and text. If it finds a supported one, it decodes it and returns a pointer to a
`proto_node` containing decoded data.

What's important to note is that you invoke a different parser than before, but
serializing the result is still done with `la_proto_tree_format_text()`. This
function is your Swiss army knife for serializing everything what libacars is
able to decode.

### Example 3: Decoding a specific application

Suppose that you have implemented your own ADS-C decoder. You want to keep using
it and use libacars only to decode FANS-1/A CPDLC.

The program might look like this:

```C
#include <stdbool.h>		// bool
#include <stdio.h>		// printf(3)
#include <string.h>		// strcmp(3), strlen(3)
#include <libacars/libacars.h>	// la_proto_node, la_proto_tree_destroy(),
				// la_proto_tree_format_text()
#include <libacars/cpdlc.h>	// la_cpdlc_parse()
#include <libacars/vstring.h>	// la_vstring, la_vstring_destroy()

int main() {
	char *label = "AA";
	char *message = "/AKLCDYA.AT1.9V-SVG21D0755D84AD067448398722949A7521C8AB4A1C8EAB5CE393";
// Assume your clever parser has split the message text into following fields:
	char *ground_addr = "AKLCDYA";
	char *imi = "AT1";
	char *air_addr = ".9V-SVG";
// Hex string converted to raw bytes. Trailing \xE3\x93 cut off (this is CRC).
	uint8_t *data = "\x21\xD0\x75\x5D\x84\xAD\x06\x74\x48\x39\x87\x22\x94\x9A\x75\x21\xC8\xAB\x4A\x1C\x8E\xAB\x5C";
	la_msg_dir direction = LA_MSG_DIR_GND2AIR;

// Determine the message type from the IMI field
	if(!strcmp(imi, "ADS")) {
		/* It's an ADS-C message - launch our built-in decoder here */
	} else if(
		!strcmp(imi, "CR1") ||
		!strcmp(imi, "CC1") ||
		!strcmp(imi, "DR1") ||
		!strcmp(imi, "AT1")) {
// It's a CPDLC message - decode it with libacars
		la_proto_node *node = la_cpdlc_parse(data, strlen(data), direction);
		if(node != NULL) {
			la_vstring *vstr = la_proto_tree_format_text(NULL, node);
			printf("Decoded CPDLC message:\n%s\n", vstr->str);
			la_vstring_destroy(vstr, true);
		} else {
			printf("CPDLC decoder failed\n");
		}
		la_proto_tree_destroy(node);
	} else {
		printf("Unsupported ACARS application\n");
	}
}
```
This time we need `<libacars/cpdlc.h>` to get the function `la_cpdlc_parse()`
which is a CPDLC message parser. As expected, it returns a pointer to
`la_proto_node` which can be serialized with `la_proto_tree_format_text()`.

Result:

```
$ ./decode_cpdlc
Decoded CPDLC message:
CPDLC Uplink Message:
 Header:
  Msg ID: 3
  Timestamp: 20:07:21
 Message data:
  AT [position] CONTACT [icaounitname] [frequency]
   Fix: VANDA
   Facility Name: CHRISTCHURCH
   Facility function: control
   VHF: 128.100 MHz
```

### Accessing message fields

Having only an option to pretty-print decoded messages would be quite a
limitation. What's really cool is to access the values of data fields to do all
sorts of stuff with them - print them in a different format, analyze them, load
them into a database, right? Good news - each and every data field decoded by
libacars is accessible to the application.  It's just a matter of including
appropriate headers and getting acquainted with relevant data structures defined
there.

First, let's see how the `la_proto_node` structure looks like:

```C
typedef struct la_proto_node la_proto_node;

struct la_proto_node {
        la_type_descriptor const *td;
        void *data;
        la_proto_node *next;
};
```

The `next` field, being a pointer to a structure of the same type, suggests it's
a building block for a single-linked list, which indeed it is.  Recall yourself
the example protocol tree from "General API concepts" section. After the whole
chain of messages is decoded, it is represented by a linked list of
`la_proto_node`s.

`void *data` is an opaque pointer to a structure containing the actual message
data. This is what you want to access - but how? Well, the case with opaque
pointers is that they have to be cast to something meaningful first. The
`la_type_descriptor const *td` field tells what type of message this
`proto_node` contains. Having this information, we know what is the data type we
shoud cast `void *data` to.

For every protocol supported by libacars there exists a global constant of type
`la_type_descriptor` named `la_DEF_<protocol_name>_message`. Let's call this
constant a type descriptor of a particular protocol. After the message is
decoded, the `td` pointer in each `la_proto_node` structure points to a type
descriptor of a protocol and therefore tells what kind of message is stored in
this protocol node.

Suppose we have a `la_proto_node *node` pointer pointing to a valid protocol
node. The simplest way of figuring out what's stored inside it might look as
follows:

```C
if(node->td == &la_DEF_acars_message) {
	/* It's an ACARS message */
} else if(node->td == &la_DEF_arinc_message) {
	/* It's an ARINC-622 message */
} else if(node->td == &la_DEF_adsc_message) {
	/* It's an ADS-C message */
} else if(node->td == &la_DEF_cpdlc_message) {
	/* It's a CPDLC message */
} else {
	/* None of the above */
}
```

As the decoded protocol tree usually consists of several protocol nodes chained
together, one has to walk the entire tree first to find the proto_node
containing the data of interest. There is a tool for that:

```C
#include <libacars/libacars.h>
/* ... */
la_proto_node *la_proto_tree_find_protocol(la_proto_node *root, la_type_descriptor const * const td);
```

Suppose you have a decoded protocol tree pointed to by `la_proto_node
*my_tree_root` and you want to read some ADS-C fields from it.  You can find the
ADS-C node in the tree with:

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>
/* ... */
la_proto_node *adsc_node = la_proto_tree_find_protocol(my_tree_root, &la_DEF_adsc_message);
```

or using a convenience function:

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>
/* ... */
la_proto_node *adsc_node = la_proto_tree_find_adsc(my_tree_root);
```

If there is no ADS-C message in the tree, the call will return NULL.

Similar convenience functions are provided for each supported protocol.

#### Accessing ACARS fields

Once you've located the protocol node containing the ACARS data (which is most
often the top one in the tree anyway), just cast the `data` field to
`la_acars_msg *` and access its fields as you would do with any structure.

Here is how to get the flight number from the ACARS header:

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>
/* ... */
la_proto_node *acars_node = la_proto_tree_find_acars(my_tree_root);
if(acars_node != NULL) {
	la_acars_msg *acars_message = (la_acars_msg *)acars_node->data;
	printf("Flight number is: %s\n", acars_message->flight_id;
}
```

Refer to the [API Reference](API_REFERENCE.md) for a full list of fields and their types.

#### Accessing ARINC-622 fields

This is similar to ACARS, except that the `data` pointer must be cast to
`la_arinc_msg *`. For example, here is how to get the ground facility address:

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>
/* ... */
la_proto_node *arinc_node = la_proto_tree_find_arinc(my_tree_root);
if(arinc_node != NULL) {
	la_arinc_msg *arinc_message = (la_arinc_msg *)arinc_node->data;
	printf("Ground address is: %s\n", arinc_message->gs_addr;
}
```

Refer to the [API Reference](API_REFERENCE.md) for a full list of fields and their types.

#### Accessing ADS-C fields

First you need to include `<libacars/adsc.h>` and cast the `data` pointer to
`la_adsc_msg_t *`. Then things become a bit more complicated than for ACARS or
ARINC-622, because ADS-C messages have their own hierarchy consisting of tags
(groups of parameters) and parameter values themselves. Some guidelines:

- The `tag_list` field in `la_adsc_msg_t` is a pointer to the first tag in the
  message. The tag list is a single-linked list implemented as a `la_list` type.
  Refer to the [API Reference](API_REFERENCE.md) for a complete description of this type and
  related functions.
- To access a particular parameter, you have to know the tag under which it
  exists (refer to definitions of tag types in `<libacars/adsc.h>`). Then you
  have to find the appropriate tag in the tag list (making sure that the tag
  actually exists in the message).
- Once you've found the tag of interest, cast the `data` pointer in `la_list`
  to a pointer of the appropriate tag type and read the fields as usual.

For a working example, refer to `src/examples/adsc_get_position.c`. This program
reads ARINC-622 messages from command line or standard input, decodes them and
extracts aircraft positional data from ADS-C Basic Report tag (provided that the
message is actually an ADS-C message containing a Basic Report).

#### Accessing CPDLC fields

First you need to include `<libacars/cpdlc.h>` and cast the `data` pointer to
`la_cpdlc_msg *`.

Then the hard stuff begins.

The syntax of CPDLC messages is described using Abstract Syntax Notation One
(ASN.1). Refer to `src/libacars/fans-cpdlc.asn1` file for a complete formal
specification. Most of the C code responsible for decoding these messages is
generated automatically using ASN1-to-C compiler [asn1c](https://github.com/vlm/asn1c).
The generated API may look daunting, but this comes from the fact that the ASN.1
compiler is generic, ie. it should be able to deal with any ASN.1 module,
provided that it is syntactically correct.

Each and every data type described in fans-cpdlc.asn1 has it's own C header file
in the subdirectory `<libacars/asn1/>`. Each type also has its own type
descriptor - a global variable of type `asn_TYPE_descriptor_t`.

For a working example, refer to `src/examples/cpdlc_get_position.c`. This program
reads ARINC-622 messages from command line or standard input, decodes them and
extracts aircraft positional data from DM48 Position Report messages (provided
that the message is actually a CPDLC message of type DM48).

A comprehensive introduction to the API generated by asn1c can be found in
[asn1c documentation](https://github.com/vlm/asn1c/blob/master/doc/asn1c-usage.pdf).

// vim: textwidth=80
