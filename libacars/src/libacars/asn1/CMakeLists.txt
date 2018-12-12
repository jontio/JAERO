include (CMakeLists.inc)
if (EMIT_ASN_DEBUG)
	set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DEMIT_ASN_DEBUG=1")
endif()

# force inclusion of alloca.h from stdlib.h on macos
remove_definitions(-D_XOPEN_SOURCE=600)

add_library (asn1 OBJECT ${ASN_MODULE_SOURCES})
set_property(TARGET asn1 PROPERTY POSITION_INDEPENDENT_CODE ON)

install (FILES ${ASN_MODULE_HEADERS} DESTINATION include/libacars/asn1)
