# Helper to find and configure MPI for VTK targets. Centralize the logic for
# any necessary compiler definitions, linking etc.
find_package(MPI REQUIRED)
mark_as_advanced(MPI_LIBRARY MPI_EXTRA_LIBRARY)
include_directories(${MPI_C_INCLUDE_PATH})
# Needed for MPICH 2
add_definitions("-DMPICH_IGNORE_CXX_SEEK")

# Function to link a VTK target to the necessary MPI libraries.
function(vtk_mpi_link target)
  target_link_libraries(${target} LINK_PRIVATE ${MPI_C_LIBRARIES})
  if(MPI_CXX_LIBRARIES)
    target_link_libraries(${target} LINK_PRIVATE ${MPI_CXX_LIBRARIES})
  endif()
endfunction()
