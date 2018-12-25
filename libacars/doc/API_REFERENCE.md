# libacars API Reference

API version: 1.0

Copyright (c) 2018 Tomasz Lemiech <szpajder@gmail.com>

## Basic data types

### la_msg_dir

Indicates the direction in which the message has been sent, ie. whether it's an
uplink or a downlink message. Decoders of bit-oriented applications need this
information to decode the message correctly and unambiguously.


```C
#include <libacars/libacars.h>

typedef enum {
        LA_MSG_DIR_UNKNOWN,
        LA_MSG_DIR_GND2AIR,
        LA_MSG_DIR_AIR2GND
} la_msg_dir;
```

### la_type_descriptor

A structure describing a particular message data type. It provides methods
(pointers to functions) which can perform various tasks on data of this type.

```C
#include <libacars/libacars.h>

typedef void (la_print_type_f)(la_vstring * const vstr, void const * const data, int indent);
typedef void (la_destroy_type_f)(void *data);

typedef struct {
        la_print_type_f *format_text;
        la_destroy_type_f *destroy;
} la_type_descriptor;
```

- `la_print_type_f *format_text` - a pointer to a function which serializes the
  message of this type into a human-readable text.
- `la_destroy_type_f *destroy` - a pointer to a function which deallocates the
  memory used by a variable of this type. If the variable is of a simple type
  (eg. a scalar variable or a flat struct), then it can be freed by a simple
  call to `free()`. In this case `destroy` pointer is NULL.

It is not advised to invoke methods from `la_type_descriptor` directly.
`la_proto_tree_format_text()` and `la_proto_tree_destroy()` wrapper functions
shall be used instead.

### la_proto_node

A structure representing a single protocol node.

```C
#include <libacars/libacars.h>

typedef struct la_proto_node la_proto_node;

struct la_proto_node {
        la_type_descriptor const *td;
        void *data;
        la_proto_node *next;
};
```

- `la_type_descriptor const *td` - a pointer to a type descriptor variable
  which describes the protocol carried in this protocol node. Basically, it
  tells what `void *data` points to.
- `void *data` - an opaque pointer to a protocol-specific structure containing
  decoded message data.
- `la_proto_node *next` - a pointer to the next protocol node in this protocol
  tree (or NULL if there are no more nested protocol nodes present).

## Core protocol-agnostic API

### la_proto_node_new()

```C
#include <libacars/libacars.h>

la_proto_node *la_proto_node_new();
```

Allocates a new `la_proto_node` structure.

This function is rarely needed in user programs because protocol decoders
themselves allocate memory for returned protocol node.

### la_proto_tree_format_text()

```C
#include <libacars/libacars.h>

la_vstring *la_proto_tree_format_text(la_vstring *vstr, la_proto_node const * const root)
```

Walks the whole protocol tree pointed to by `root`, serializing each node into
human-readable text and appending the result to the variable-length string
pointed to by `vstr`. If `vstr` is NULL, the function stores the result in a
newly allocated variable-length string (which should be later freed by the
caller using `la_proto_tree_destroy()`.

### la_proto_tree_destroy()

```C
#include <libacars/libacars.h>

void la_proto_tree_destroy(la_proto_node *root);
```

Walks the whole protocol tree pointed to by `root`, freeing memory allocated
for each node. If `root` is NULL, the function does nothing.

### la_proto_tree_find_protocol()

```C
#include <libacars/libacars.h>

la_proto_node *la_proto_tree_find_protocol(la_proto_node *root, la_type_descriptor const * const td);
```

Walks the whole protocol tree pointed to by `root`, and searches for a node of a
type described by the type descriptor `*td`, Returns a pointer to the first
found protocol node which matches the given type.  Returns NULL if no matching
node has been found.

## ACARS API

### la_acars_msg

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

typedef struct {
        bool crc_ok;
        bool err;
        char mode;
        char reg[8];
        char ack;
        char label[3];
        char block_id;
        char no[5];
        char flight_id[7];
        char *txt;
} la_acars_msg;
```

A structure representing a decoded ACARS message.

- `crc_ok` - `true` if ACARS CRC verification succeeded, `false` otherwise.
- `err` - `true` if the decoder failed to decode the message. Values of other
  fields are left uninitialized.
- `mode` - mode character
- `reg` - aircraft registration number (NULL-terminated)
- `ack` - acknowledgment character
- `label` - message label (NULL-terminated)
- `block_id` - block ID
- `no` - message number (NULL-terminated)
- `flight_id` - flight number (NULL-terminated)
- `txt` - message text (NULL-terminated)

### la_acars_parse()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

la_proto_node *la_acars_parse(uint8_t *buf, int len, la_msg_dir msg_dir);
```

Takes a buffer of `len` bytes pointed to by `buf` and attempts to decode it as
an ACARS message. If there are any supported nested protocols in the ACARS
message text, they will be decoded as well.

The buffer shall start with the first byte **after** the initial SOH byte
(`'\x01'`) and should end with DEL byte (`'\x7f'`). `msg_dir` shall indicate the
direction of the transmission.  If it's set to `LA_MSG_DIR_UNKNOWN`, the
function will determine it using block ID field in the ACARS header.

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point at a `la_acars_msg` structure. The function performs
ACARS CRC verification and stores the result in the `crc_ok` field. If the
message could not be decoded, the `err` flag will be set to true.

### la_acars_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

void la_acars_format_text(la_vstring *vstr, void const * const data, int indent);
```

Serializes a decoded ACARS message pointed to by `data` into a human-readable
text indented by `indent` spaces and appends the result to `vstr` (which must be
non-NULL).

Use this function if you want to serialize only the ACARS protocol node and not
its child nodes. In most cases `la_proto_tree_format_text()` should be used
instead.

### la_acars_decode_apps()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

la_proto_node *la_acars_decode_apps(char const * const label,
        char const * const txt, la_msg_dir const msg_dir);
```

Tries to determine the ACARS application using message label stored in `label`. If
the label corresponds to any supported applications, respective application
decoders are executed in sequence to decode the message text `txt`. `msg_dir`
must be set to a correct transmission direction for the decoding to succeed.

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. If none of the application
decoders recognized the message, the function returns NULL.

### la_proto_tree_find_acars()

```C
#include <libacars/libacars.h>
#include <libacars/acars.h>

la_proto_node *la_proto_tree_find_acars(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing an ACARS message (ie. having a type descriptor of
`la_DEF_acars_message`). If `root` is NULL or no matching protocol node has been
found in the tree, the function returns NULL.

## ARINC-622 API

ARINC-622 describes a generic format for carrying Air Traffic Services (ATS)
applications in ACARS message text. The API is defined in `<libacars/arinc.h>`.
It might be used in a program which already performs basic ACARS decoding.
Refer to the [Programmer's Guide](PROG_GUIDE.md) and to `src/examples/decode_arinc.c`
for a working example which uses this API.

### la_arinc_msg

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>

typedef struct {
        char gs_addr[8];
        char air_reg[8];
        la_arinc_imi imi;
        bool crc_ok;
} la_arinc_msg;
```

A structure representing a decoded ARINC-622 message.

- `gs_addr` - ground facility address (NULL-terminated)
- `air_reg` - aircraft address (NULL-terminated)
- `imi` - an enumerated value mapped from the Imbedded Message Identifier (IMI);
  refer to `<libacars/arinc.h>` for a definition of `la_arinc_imi` type
- `crc_ok` - `true` if ARINC-622 CRC verification succeeded, `false` otherwise.

### la_arinc_parse()

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>

la_proto_node *la_arinc_parse(char const *txt, la_msg_dir const msg_dir);
```

Attempts to parse the message text `txt` sent in direction `msg_dir` as an
ARINC-622 message. Performs the following tasks:

- splits the message text into a set of fields:

  - ground facility address
  - Imbedded Message Identifier (IMI)
  - aircraft address (registration number)

- if the IMI indicates a supported bit-oriented application:

  - verifies the CRC of the message
  - converts the hex string embedded in the message text into an octet string
  - parses the octet string with an application-specific parser

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point at a `la_arinc_msg` structure. The result of ARINC
CRC verification is stored in the `crc_ok` boolean field. If the message text
does not represent an ARINC-622 application message or if the IMI field value is
unknown, the function returns NULL.

libacars currently supports the following IMIs: CR1, CC1, DR1, AT1, ADS, DIS.

### la_arinc_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>

void la_arinc_format_text(la_vstring * const vstr, void const * const data, int indent);
```

Serializes a decoded ARINC-622 message pointed to by `data` into a human-readable
text indented by `indent` spaces and appends the result to `vstr` (which must be
non-NULL).

Use this function if you want to serialize only the ARINC protocol node and not
its child nodes. In most cases `la_proto_tree_format_text()` should be used
instead.

### la_proto_tree_find_arinc()

```C
#include <libacars/libacars.h>
#include <libacars/arinc.h>

la_proto_node *la_proto_tree_find_arinc(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing an ARINC-622 message (ie. having a type descriptor of
`la_DEF_arinc_message`). If `root` is NULL or no matching protocol node has been
found in the tree, the function returns NULL.

## ADS-C API

### la_adsc_msg_t

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

typedef struct {
        bool err;
        la_list *tag_list;
} la_adsc_msg_t;
```

A structure representing a decoded ADS-C message.

- `err` - `true` if the decoder failed to decode the message, `false` otherwise.
- `tag_list` - a pointer to a `la_list` of decoded ADS-C tags (structures of
  type `la_adsc_tag_t`).

### la_adsc_tag_t

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

typedef struct {
        uint8_t tag;
        la_adsc_type_descriptor_t *type;
        void *data;
} la_adsc_tag_t;
```

A generic ADS-C tag structure.

- `tag` - tag ID
- `type` - a pointer to an ADS-C tag descriptor type which provides a textual
  label for the tag and basic methods for manipulating the tag. This is mostly
  for libacars internal use.
- `data` - an opaque pointer to a structure representing a decoded tag. Refer to
  `<libacars/adsc.h>` for definitions of tag structures (`la_adsc_basic_report_t`,
  `la_adsc_flight_id_t` and so forth).

### la_adsc_parse()

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

la_proto_node *la_adsc_parse(uint8_t *buf, int len, la_msg_dir msg_dir, la_arinc_imi imi);
```

Parses the octet string pointed to by `buf` having a length of `len` octets as
an ADS-C message sent in the direction indicated by `msg_dir`. The value of
`imi` indicates the value of the IMI field in the ARINC-622 message and must
have a value of either `ARINC_MSG_ADS` or `ARINC_MSG_DIS`.

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point at a `la_adsc_msg_t` structure.  If the message could
not be decoded, the `err` flag will be set to true.

### la_adsc_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

void la_adsc_format_text(la_vstring * const vstr, void const * const data, int indent);
```

Serializes a decoded ADS-C message pointed to by `data` into a human-readable
text indented by `indent` spaces and appends the result to `vstr` (which must be
non-NULL). As the ADS-C protocol node is always the leaf (terminating) node in
the protocol tree, this function will have the same effect as
`la_proto_tree_format_text()` which should be used instead in most cases.

### la_adsc_destroy()

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

void la_adsc_destroy(void *data);
```

Deallocates memory used by the protocol node `data` and all the child
structures. `data` must be a pointer to a `la_adsc_msg_t` structure. Rather than
this function, you should use `la_proto_tree_destroy()` instead.

### la_proto_tree_find_adsc()

```C
#include <libacars/libacars.h>
#include <libacars/adsc.h>

la_proto_node *la_proto_tree_find_adsc(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing an ADS-C message (ie. having a type descriptor of
`la_DEF_adsc_message`). If `root` is NULL or no matching protocol node has been
found in the tree, the function returns NULL.

## CPDLC API

Basic CPDLC API is defined in `<libacars/cpdlc.h>`. This is enough to perform
typical operations, like decoding and serializing decoded CPDLC messages into
human-readable text. Advanced API is also available in `<libacars/asn1/*.h>`. It
is used when there is a need to access individual fields of decoded CPDLC
messages.

### la_cpdlc_msg

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

typedef struct {
        asn_TYPE_descriptor_t *asn_type;
        void *data;
        bool err;
} la_cpdlc_msg;
```

A top-level structure representing a decoded CPDLC message.

- `asn_type` - a descriptor of a top-level ASN.1 data type contained in `data`.
- `data` - an opaque pointer to a decoded ASN.1 structure of the message
- `err` - `true` if the decoder failed to decode the message, `false` otherwise.

### la_cpdlc_parse()

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

la_proto_node *la_cpdlc_parse(uint8_t *buf, int len, la_msg_dir const msg_dir);
```

Parses the octet string pointed to by `buf` having a length of `len` octets as
a FANS-1/A CPDLC message sent in the direction indicated by `msg_dir`.

The function returns a pointer to a newly allocated `la_proto_node` structure
which is the root of the decoded protocol tree. The `data` pointer of the top
`la_proto_node` will point at a `la_cpdlc_msg` structure.  If the message could
not be decoded, the `err` flag will be set to true.

It is imperative to specify correct value for `msg_dir`. This is because CPDLC
messages are encoded using ASN.1 Packed Encoding Rules (PER). PER does not
encode ASN.1 tags, so the message type cannot be determined based on its
contents. Supplying a wrong message direction will (at best) cause the message
decoding to fail (`err` flag in `la_cpdlc_msg` structure will be set to `true`),
but it may also happen that the decoder will produce a perfectly valid result,
which is in fact different to what has been transmitted by the sender due to
encoding ambiguity (some encoded messages can be successfully decoded both as
CPDLC uplink and downlink message).

`la_cpdlc_parse()` sets the fields in the resulting `la_cpdlc_msg` structure as
follows:

- for air-to-ground messages:
  - `asn_type` is set to `&asn_DEF_FANSATCDownlinkMessage`
  - `data` pointer type is `FANSATCDownlinkMessage_t *`
- for ground-to-air messages:
  - `asn_type` is set to `&asn_DEF_FANSATCUplinkMessage`
  - `data` pointer type is `FANSATCUplinkMessage_t *`

These types are defined in the respective header files in `<libacars/asn1/...>`
include directory.

### la_cpdlc_format_text()

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

void la_cpdlc_format_text(la_vstring * const vstr, void const * const data, int indent);
```

Serializes a decoded CPDLC message pointed to by `data` into a human-readable
text indented by `indent` spaces and appends the result to `vstr` (which must be
non-NULL). As the CPDLC protocol node is always the leaf (terminating) node in
the protocol tree, this function will have the same effect as
`la_proto_tree_format_text()` which should be used instead of this function in
most cases.

### la_cpdlc_destroy()

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

void la_cpdlc_destroy(void *data);
```

Deallocates memory used by the protocol node `data` and all the child
structures. `data` must be a pointer to a `la_cpdlc_msg` structure. Rather than
this function, you should use `la_proto_tree_destroy()` instead.

### la_proto_tree_find_cpdlc()

```C
#include <libacars/libacars.h>
#include <libacars/cpdlc.h>

la_proto_node *la_proto_tree_find_cpdlc(la_proto_node *root);
```

Walks the protocol tree pointed to by `root` and returns a pointer to the first
encountered node containing a CPDLC message (ie. having a type descriptor of
`la_DEF_cpdlc_message`). If `root` is NULL or no matching protocol node has been
found in the tree, the function returns NULL.

## la_vstring API

libacars uses `la_vstring` data type for storing results of message
serialization. It is essentially a variable-length, appendable, auto-growing
NULL-terminated string. It is generic and can therefore be used for any purpose
not necessarily related to ACARS processing.

### la_vstring

```C
#include <libacars/vstring.h>

typedef struct {
        char *str;
        size_t len;
        size_t allocated_size;
} la_vstring;
```

- `str` - the pointer to the string buffer. You may access it directly for
  reading (eg. to print it) but do not write it directly, use provided functions
  instead.
- `len` - current length of the string (not including trailing '\0')
- `allocated_size` - current size of the allocated buffer

### la_vstring_new()

```C
#include <libacars/vstring.h>

la_vstring *la_vstring_new();
```

Allocates a new `la_vstring` and returns a pointer to it.

### la_vstring_destroy()

```C
#include <libacars/vstring.h>

void la_vstring_destroy(la_vstring *vstr, bool destroy_buffer);
```

Frees the memory used by the `la_vstring` pointed to by `vstr`. If
`destroy_buffer` is set to true, frees the character buffer `str` as well.
Otherwise the character buffer and its contents are preserved and may be used
later (but remember to copy the pointer to it before executing `la_vstring_destroy()`
and free it later with `free()`.

### la_vstring_append_sprintf()

```C
#include <libacars/vstring.h>

void la_vstring_append_sprintf(la_vstring * const vstr, char const *fmt, ...)
```
Appends formatted string to the end of `vstr`.  Automatically extends `vstr` if
it's too short to fit the result.

### la_vstring_append_buffer()

```C
#include <libacars/vstring.h>

void la_vstring_append_buffer(la_vstring * const vstr, void const * buffer, size_t size);
```

Appends the contents of `buffer` of length `size` to the end of `vstr`.
Automatically extends `vstr` if it's too short to fit the result. Use this
function to append non-NULL-terminated strings to the `la_vstring`.

## la_list API

`la_list` is a single-linked list.

### la_list

```C
#include <libacars/list.h>

typedef struct la_list la_list;

struct la_list {
        void *data;
        la_list *next;
};
```

- `data` - an opaque pointer to data element stored in the list item
- `next` - a pointer to the next item in the list

### la_list_append()

```C
#include <libacars/list.h>

la_list *la_list_append(la_list *l, void *data);
```

Allocates a new list item, stores `data` in it and appends it at the end of the
list `l`. Returns a pointer to the top element of the list, ie. when `l` is not
NULL, it returns `l`, otherwise returns a pointer to the newly allocated list
item.

### la_list_next()

```C
#include <libacars/list.h>

la_list *la_list_next(la_list const * const l);
```

A convenience function which returns a pointer to the list element occuring
after `l`. Essentially, it returns `l->next`. Returns NULL, if `l` is the last
item in the list.  Useful when iterating over list elements in a `for()` or
`while()` loop (but see also `la_list_foreach()`).

### la_list_length()

```C
#include <libacars/list.h>

size_t la_list_length(la_list const *l);
```

Returns the number of items in the list `l`. If `l` is NULL, returns 0.

### la_list_foreach()

```C
#include <libacars/list.h>

void la_list_foreach(la_list *l, void (*cb)(), void *ctx);
```

Iterates over items of the list `l` and executes a callback function `cb` for
each item:

```C
cb(l->data, ctx);
```

- `l->data` - a pointer to the data chunk stored in the current list item
- `ctx` - an opaque pointer to an arbitrary context data (might be NULL)

### la_list_free()

```C
#include <libacars/list.h>

void la_list_free(la_list *l);
```

Deallocates memory used by all items in the list `l`. All data chunks stored in
the list are freed with `free(3)`. This is appropriate when the data chunks are
simple data structures allocated with a single `malloc(3)` call.

### la_list_free_full()

```C
#include <libacars/list.h>

void la_list_free_full(la_list *l, void (*node_free)());
```

Deallocates memory used by all items in the list `l`. All data chunks stored in
the list are freed by executing callback function `node_free`:

```C
node_free(l->data);
```

- `l->data` - a pointer to the data chunk stored in the current list item

This function should be used when data chunks are complex structures composed of
multiple allocated memory chunks.

## libacars configuration parameters

libacars provides a global variable `la_config`. It is a structure containing
configuration parameters affecting the operation of library components.
Programs can directly set these parameters to modify libacars behaviour.

```C
#include <libacars/libacars.h>

typedef struct {
        bool dump_asn1;
} la_config_struct;

extern la_config_struct la_config;
```

To change the value of a chosen parameter:

```C
#include <libacars/libacars.h>
/* ... */
la_config.dump_asn1 = true;
```

Available parameters:

- `dump_asn1` - when set to `true`, the CPDLC message serializer will produce
  raw dump of the whole ASN.1 structure in addition to its standard
  human-readable text output. This might be useful for debugging the library or
  just to increase output verbosity at user's discretion.

// vim: textwidth=80
