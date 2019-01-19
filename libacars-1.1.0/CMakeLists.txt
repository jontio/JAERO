cmake_minimum_required (VERSION 3.1)
project (libacars C)

set (LA_VERSION_MAJOR 1)
set (LA_VERSION_MINOR 1)
set (LA_VERSION_PATCH 0)
set (LA_VERSION "${LA_VERSION_MAJOR}.${LA_VERSION_MINOR}.${LA_VERSION_PATCH}"
)
set (CMAKE_C_STANDARD 11)
set (C_STANDARD_REQUIRED ON)
set (CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
   message(STATUS "Build type not specified: defaulting to Release")
endif(NOT CMAKE_BUILD_TYPE)

option(EMIT_ASN_DEBUG, "Enable debugging of ASN.1 decoder (requires
CMAKE_BUILD_TYPE=Debug" OFF)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

include_directories (
	${PROJECT_SOURCE_DIR}/src
)

add_subdirectory (src)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	@ONLY
)

#only add uninstall target if this is the top project
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()

# build a CPack driven installer package
include (InstallRequiredSystemLibraries)
set (CPACK_RESOURCE_FILE_LICENSE
	"${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set (CPACK_RESOURCE_FILE_README
	"${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set (CPACK_PACKAGE_VERSION_MAJOR "${LA_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${LA_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${LA_VERSION_PATCH}")
set (CPACK_PACKAGE_CHECKSUM "SHA256")
set (CPACK_STRIP_FILES TRUE)
if (UNIX)
	set (CPACK_GENERATOR "TGZ")
elseif (WIN32)
	set (CPACK_GENERATOR "ZIP")
endif ()
include (CPack)
