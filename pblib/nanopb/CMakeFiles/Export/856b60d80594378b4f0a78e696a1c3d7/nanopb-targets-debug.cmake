#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "nanopb::protobuf-nanopb-static" for configuration "Debug"
set_property(TARGET nanopb::protobuf-nanopb-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(nanopb::protobuf-nanopb-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/protobuf-nanopbd.lib"
  )

list(APPEND _cmake_import_check_targets nanopb::protobuf-nanopb-static )
list(APPEND _cmake_import_check_files_for_nanopb::protobuf-nanopb-static "${_IMPORT_PREFIX}/lib/protobuf-nanopbd.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
