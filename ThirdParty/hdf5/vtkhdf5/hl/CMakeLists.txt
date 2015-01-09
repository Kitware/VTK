PROJECT (HDF5_HL C CXX)

#-----------------------------------------------------------------------------
# Apply Definitions to compiler in this directory and below
#-----------------------------------------------------------------------------
add_definitions (${HDF5_EXTRA_C_FLAGS})

#-----------------------------------------------------------------------------
# Shared Libs
#-----------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
  set (HL_BUILT_AS_DYNAMIC_LIB 1)
else (BUILD_SHARED_LIBS)
  set (HL_BUILT_AS_STATIC_LIB 1)
endif (BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# List Source files
#-----------------------------------------------------------------------------
INCLUDE_DIRECTORIES (${HDF5_HL_SOURCE_DIR}/src )

add_subdirectory (${HDF5_HL_SOURCE_DIR}/src ${HDF5_HL_BINARY_DIR}/src)

#-- Build the High level Tools
if (HDF5_BUILD_TOOLS)
  add_subdirectory (${HDF5_HL_SOURCE_DIR}/tools ${HDF5_HL_BINARY_DIR}/tools)
endif (HDF5_BUILD_TOOLS)

#-- Add High Level Examples
if (HDF5_BUILD_EXAMPLES)
  add_subdirectory (${HDF5_HL_SOURCE_DIR}/examples ${HDF5_HL_BINARY_DIR}/examples)
endif (HDF5_BUILD_EXAMPLES)

#-- Build the Unit testing if requested
if (NOT HDF5_EXTERNALLY_CONFIGURED)
  if (BUILD_TESTING)
    add_subdirectory (${HDF5_HL_SOURCE_DIR}/test ${HDF5_HL_BINARY_DIR}/test)
  endif (BUILD_TESTING)
endif (NOT HDF5_EXTERNALLY_CONFIGURED)

if (HDF5_BUILD_CPP_LIB)
  add_subdirectory (${HDF5_HL_SOURCE_DIR}/c++ ${HDF5_HL_BINARY_DIR}/c++)
endif (HDF5_BUILD_CPP_LIB)
