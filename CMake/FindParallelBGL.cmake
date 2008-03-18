# This module looks for the Parallel BGL
#
# This module will set the following variables:
#   PBGL_FOUND               TRUE if we have found the Parallel BGL
#   PBGL_INCLUDE_DIR         Include directories for the Parallel BGL headers
#                            This directory must come *before* Boost's include
#                            path. Use, e.g.,
#                              INCLUDE_DIRECTORIES(BEFORE ${PBGL_INCLUDE_DIR})
#   PBGL_LIBRARIES           Parallel BGL libraries to link against
#
# The Parallel BGL require Boost version 1.34.0 or newer, although at
# present this module does not test for Boost independently.
find_path(PBGL_INCLUDE_DIR boost/graph/distributed/adjacency_list.hpp
  DOC "Parallel BGL header include path")
find_library(PBGL_LIBRARIES boost_parallel_mpi
  DOC "Parallel BGL libraries to link against")

mark_as_advanced(PBGL_INCLUDE_DIR PBGL_LIBRARIES)

if (PBGL_INCLUDE_DIR AND PBGL_LIBRARIES)
  set(PBGL_FOUND TRUE)
endif (PBGL_INCLUDE_DIR AND PBGL_LIBRARIES)

if (PBGL_FOUND)
  if (NOT ParallelBGL_FIND_QUIETLY)
    message(STATUS "Found Parallel BGL with library ${PBGL_LIBRARIES}")
  endif (NOT ParallelBGL_FIND_QUIETLY)
else (PBGL_FOUND)
  if (ParallelBGL_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find Parallel BGL. Please set PBGL_INCLUDE_DIR and PBGL_LIBRARIES appropriately.")
  endif (ParallelBGL_FIND_REQUIRED)
endif (PBGL_FOUND)
