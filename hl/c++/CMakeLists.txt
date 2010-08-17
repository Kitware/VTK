cmake_minimum_required (VERSION 2.8)
PROJECT (HDF5_HL_CPP)

#-----------------------------------------------------------------------------
# Main HL lib is in /src
#-----------------------------------------------------------------------------
ADD_SUBDIRECTORY (${HDF5_HL_CPP_SOURCE_DIR}/src ${HDF5_HL_CPP_BINARY_DIR}/src)

# --------------------------------------------------------------------
#  Add in the examples for the Packet Table codes
# --------------------------------------------------------------------
IF (HDF5_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY (${HDF5_HL_CPP_SOURCE_DIR}/examples ${HDF5_HL_CPP_BINARY_DIR}/examples)
ENDIF (HDF5_BUILD_EXAMPLES)

# --------------------------------------------------------------------
# Add in the unit tests for the packet table c++ wrapper
# --------------------------------------------------------------------

IF (BUILD_TESTING)
  ADD_SUBDIRECTORY (${HDF5_HL_CPP_SOURCE_DIR}/test ${HDF5_HL_CPP_BINARY_DIR}/test)
ENDIF (BUILD_TESTING)
