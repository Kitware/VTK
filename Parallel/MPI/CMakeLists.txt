find_package(MPI REQUIRED)
mark_as_advanced(MPI_LIBRARY MPI_EXTRA_LIBRARY)
set(vtkParallelMPI_SYSTEM_INCLUDE_DIRS ${MPI_C_INCLUDE_PATH})

# =============================================================================
# This following block should probably go into a seperate cmake file
# --------------------------------------------------- MPI-Testing-Support-BEGIN
# Configure MPI testing support.
# FLAGS used and set for MPI testing
# VTK_MPIRUN_EXE - full path to mpirun command
# VTK_MPI_PRENUMPROC_FLAGS - flags used directly before the num. of procs flag
# VTK_MPI_NUMPROC_FLAG - flag that is used to tell this mpirun how many procs to start
# VTK_MPI_PREFLAGS - flags used directly before process to be run by mpirun
# VTK_MPI_POSTFLAGS - flags used after all other flags by mpirun
# So, tests will be run something like this:
# ${VTK_MPIRUN_EXE} ${VTK_MPI_PRENUMPROC_FLAGS} ${VTK_MPI_NUMPROC_FLAG} 2 ${VTK_MPI_PREFLAGS} executable ${VTK_MPI_POSTFLAGS}
#
# Use MPI variables defined in the CMake (2.8) FindMPI module.
if(MPIEXEC)
  set(VTK_MPIRUN_EXE ${MPIEXEC} CACHE FILEPATH "The full path to mpirun command" FORCE)
  set(VTK_MPI_PRENUMPROC_FLAGS ${MPIEXEC_PREFLAGS} CACHE STRING
    "These flags will be directly before the number of processess flag (see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)" FORCE)
  if(NOT ${MPI_NUMPROC_FLAG})
    set(VTK_MPI_NUMPROC_FLAG "-np" CACHE STRING
      "Flag used by mpi to specify the number of processes, the next option will be the number of processes. (see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)" FORCE)
  else()
    set(VTK_MPI_NUMPROC_FLAG ${MPIEXEC_NUMPROC_FLAG} CACHE STRING
      "Flag used by mpi to specify the number of processes, the next option will be the number of processes. (see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)" FORCE)
  endif()
  set(VTK_MPI_PREFLAGS ${MPIEXEC_PREFLAGS} CACHE STRING
    "These flags will be directly before the executable that is being run by VTK_MPIRUN_EXE. (see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)" FORCE)
  set(VTK_MPI_POSTFLAGS ${MPIEXEC_POSTFLAGS} CACHE STRING
    "These flags will come after all flags given to MPIRun.(see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)" FORCE)
  set(VTK_MPI_MAX_NUMPROCS ${MPIEXEC_MAX_NUMPROCS} CACHE STRING
    "Maximum number of processors available to run parallel applications. (see ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt for more info.)" FORCE)
else()
  message(FATAL_ERROR "MPIEXEC was empty.")
endif()

mark_as_advanced(
  VTK_MPI_PRENUMPROC_FLAGS
  VTK_MPI_NUMPROC_FLAG
  VTK_MPIRUN_EXE
  VTK_MPI_PREFLAGS
  VTK_MPI_POSTFLAGS
  VTK_MPI_MAX_NUMPROCS
  )

separate_arguments(VTK_MPI_PRENUMPROC_FLAGS)
separate_arguments(VTK_MPI_PREFLAGS)
separate_arguments(VTK_MPI_POSTFLAGS)

include(vtkTestingMacros)
# ----------------------------------------------------- MPI-Testing-Support-END
# =============================================================================

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

# FIXME: The ifdefs need to be removed, for now defining it to on
set(${vtk-module}_EXPORT_CODE "#define VTK_USE_MPI")

vtk_module_library(vtkParallelMPI ${Module_SRCS})
target_link_libraries(vtkParallelMPI ${MPI_C_LIBRARIES})
if (MPI_CXX_LIBRARIES)
  target_link_libraries(vtkParallelMPI ${MPI_CXX_LIBRARIES})
endif()
