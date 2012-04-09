find_package(MPI)
mark_as_advanced(MPI_LIBRARY MPI_EXTRA_LIBRARY)
include_directories(${MPI_INCLUDE_PATH})

set(Module_SRCS
  vtkMPICommunicator.cxx
  vtkMPIController.cxx
  # vtkMPIEventLog.cxx # Deprecated
  vtkMPI.h
  )

set_source_files_properties(
  vtkMPI
  HEADER_FILE_ONLY
  )

set_source_files_properties(
  vtkMPI
  WRAP_EXCLUDE
  )

# Needed for mpich 2
add_definitions("-DMPICH_IGNORE_CXX_SEEK")

vtk_module_library(vtkParallelMPI ${Module_SRCS})
target_link_libraries(vtkParallelMPI ${MPI_LIBRARY})
if(MPI_EXTRA_LIBRARY)
  target_link_libraries(vtkParallelMPI ${MPI_EXTRA_LIBRARY})
endif()
