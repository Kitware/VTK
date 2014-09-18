PROJECT (HDF5_HL)

#-----------------------------------------------------------------------------
# Shared Libs
#-----------------------------------------------------------------------------
IF (BUILD_SHARED_LIBS)
  SET (HL_BUILT_AS_DYNAMIC_LIB 1)
ENDIF ()

#-----------------------------------------------------------------------------
# List Source files
#-----------------------------------------------------------------------------
INCLUDE_DIRECTORIES (${HDF5_HL_SOURCE_DIR}/src )

ADD_SUBDIRECTORY (${HDF5_HL_SOURCE_DIR}/src ${HDF5_HL_BINARY_DIR}/src)

#-- Build the High level Tools
IF (HDF5_BUILD_TOOLS)
  ADD_SUBDIRECTORY (${HDF5_HL_SOURCE_DIR}/tools ${HDF5_HL_BINARY_DIR}/tools)
ENDIF ()

#-- Add High Level Examples
IF (HDF5_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY (${HDF5_HL_SOURCE_DIR}/examples ${HDF5_HL_BINARY_DIR}/examples)
ENDIF ()

#-- Build the Unit testing if requested
IF (NOT HDF5_EXTERNALLY_CONFIGURED)
  IF (BUILD_TESTING)
    ADD_SUBDIRECTORY (${HDF5_HL_SOURCE_DIR}/test ${HDF5_HL_BINARY_DIR}/test)
  ENDIF ()
ENDIF ()

#-- Build the High Level Fortran source codes
IF (HDF5_BUILD_FORTRAN)
  ADD_SUBDIRECTORY (fortran)
ENDIF ()

IF (HDF5_BUILD_CPP_LIB)
  ADD_SUBDIRECTORY (${HDF5_HL_SOURCE_DIR}/c++ ${HDF5_HL_BINARY_DIR}/c++)
ENDIF ()
