#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "nanopb::protobuf-nanopb-static" for configuration "MinSizeRel"
set_property(TARGET nanopb::protobuf-nanopb-static APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(nanopb::protobuf-nanopb-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "C"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/protobuf-nanopb.lib"
  )

list(APPEND _cmake_import_check_targets nanopb::protobuf-nanopb-static )
list(APPEND _cmake_import_check_files_for_nanopb::protobuf-nanopb-static "${_IMPORT_PREFIX}/lib/protobuf-nanopb.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
