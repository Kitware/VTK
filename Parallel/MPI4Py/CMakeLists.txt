include(vtkMPI)
set(vtkParallelMPI4Py_SYSTEM_INCLUDE_DIRS ${MPI_C_INCLUDE_PATH})

set(Module_SRCS
  vtkMPI4PyCommunicator.cxx
  )

if(VTK_USE_SYSTEM_MPI4PY)
  if(NOT MPI4PY_INCLUDE_DIR)
    execute_process(
      COMMAND "${PYTHON_EXECUTABLE}" -c
              "import mpi4py; print(mpi4py.get_include())"
      OUTPUT_VARIABLE mpi4py_include_dir
      OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    set(MPI4PY_INCLUDE_DIR "${mpi4py_include_dir}"
      CACHE PATH "Path the the MPI4Py include directory")
    mark_as_advanced(MPI4PY_INCLUDE_DIR)
  endif()
else()
  set(MPI4PY_INCLUDE_DIR "${vtkmpi4py_SOURCE_DIR}/vtkmpi4py/src/include")
endif()
include_directories("${MPI4PY_INCLUDE_DIR}")

# Needed for mpich 2
add_definitions("-DMPICH_IGNORE_CXX_SEEK")

# FIXME: The ifdefs need to be removed, for now defining it to on
set(${vtk-module}_EXPORT_CODE "#define VTK_USE_MPI")

vtk_module_library(vtkParallelMPI4Py ${Module_SRCS})
vtk_mpi_link(vtkParallelMPI4Py)
