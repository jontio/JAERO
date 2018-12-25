add_executable (decode_arinc decode_arinc.c)
add_executable (adsc_get_position adsc_get_position.c)
add_executable (cpdlc_get_position cpdlc_get_position.c)
target_link_libraries (decode_arinc acars)
target_link_libraries (adsc_get_position acars)
target_link_libraries (cpdlc_get_position acars m)
install (TARGETS
	decode_arinc
	adsc_get_position
	cpdlc_get_position
	DESTINATION bin
)
