include(vtkMPI)
set(vtkParallelMPI_SYSTEM_INCLUDE_DIRS ${MPI_C_INCLUDE_PATH})

include(vtkTestingMPISupport)
include(vtkTestingMacros)

set(Module_SRCS
  vtkMPICommunicator.cxx
  vtkMPIController.cxx
  vtkMPIUtilities.cxx
  # vtkMPIEventLog.cxx # Deprecated
  vtkMPI.h
  )

set_source_files_properties(
  vtkMPI
  HEADER_FILE_ONLY
  )

set_source_files_properties(
  vtkMPI
  vtkMPIUtilities
  PROPERTIES
    WRAP_EXCLUDE 1
    WRAP_EXCLUDE_PYTHON 1
  )

# Needed for mpich 2
add_definitions("-DMPICH_IGNORE_CXX_SEEK")

# FIXME: The ifdefs need to be removed, for now defining it to on
set(${vtk-module}_EXPORT_CODE "#define VTK_USE_MPI")

vtk_module_library(vtkParallelMPI ${Module_SRCS})
vtk_mpi_link(vtkParallelMPI)
