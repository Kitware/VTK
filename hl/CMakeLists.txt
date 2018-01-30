# XXX(kitware): cmake versions are handled by VTK.
if (FALSE)
cmake_minimum_required (VERSION 3.2.2)
endif ()
PROJECT (HDF5_HL C CXX)

#-----------------------------------------------------------------------------
# Apply Definitions to compiler in this directory and below
#-----------------------------------------------------------------------------
add_definitions (${HDF_EXTRA_C_FLAGS})


#-----------------------------------------------------------------------------
# List Source files
#-----------------------------------------------------------------------------
INCLUDE_DIRECTORIES (${HDF5_HL_SOURCE_DIR}/src )

add_subdirectory (src)

#-- Build the High level Tools
if (HDF5_BUILD_TOOLS)
  add_subdirectory (tools)
endif ()

#-- Add High Level Examples
if (HDF5_BUILD_EXAMPLES)
  add_subdirectory (examples)
endif ()

#-- Build the Unit testing if requested
if (NOT HDF5_EXTERNALLY_CONFIGURED)
  if (BUILD_TESTING)
    add_subdirectory (test)
  endif ()
endif ()
