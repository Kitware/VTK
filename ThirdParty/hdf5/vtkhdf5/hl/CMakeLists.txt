if (FALSE) # XXX(kitware): cmake versions are handled by VTK.
cmake_minimum_required (VERSION 3.10)
endif ()
project (HDF5_HL C)

#-----------------------------------------------------------------------------
# List Source files
#-----------------------------------------------------------------------------

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
