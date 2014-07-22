cmake_minimum_required (VERSION 2.8.11)
PROJECT (HDF5_HL_CPP)

#-----------------------------------------------------------------------------
# Main HL lib is in /src
#-----------------------------------------------------------------------------
add_subdirectory (${HDF5_HL_CPP_SOURCE_DIR}/src ${HDF5_HL_CPP_BINARY_DIR}/src)

# --------------------------------------------------------------------
#  Add in the examples for the Packet Table codes
# --------------------------------------------------------------------
if (HDF5_BUILD_EXAMPLES)
  add_subdirectory (${HDF5_HL_CPP_SOURCE_DIR}/examples ${HDF5_HL_CPP_BINARY_DIR}/examples)
endif (HDF5_BUILD_EXAMPLES)

# --------------------------------------------------------------------
# Add in the unit tests for the packet table c++ wrapper
# --------------------------------------------------------------------

if (BUILD_TESTING)
  add_subdirectory (${HDF5_HL_CPP_SOURCE_DIR}/test ${HDF5_HL_CPP_BINARY_DIR}/test)
endif (BUILD_TESTING)
