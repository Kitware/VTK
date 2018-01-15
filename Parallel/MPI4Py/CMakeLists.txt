include(vtkMPI)
set(vtkParallelMPI4Py_SYSTEM_INCLUDE_DIRS ${MPI_C_INCLUDE_PATH})

set(Module_SRCS
  vtkMPI4PyCommunicator.cxx
  )

if(NOT MPI4PY_INCLUDE_DIR)
  execute_process(
    COMMAND "${PYTHON_EXECUTABLE}" -c
            "import mpi4py; print(mpi4py.get_include())"
    RESULT_VARIABLE mpi4py_include_dir_res
    OUTPUT_VARIABLE mpi4py_include_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  if (NOT mpi4py_include_dir_res)
    set(MPI4PY_INCLUDE_DIR "${mpi4py_include_dir}"
      CACHE PATH "Path of the MPI4Py include directory")
  endif ()
endif()
if (MPI4PY_INCLUDE_DIR)
  mark_as_advanced(MPI4PY_INCLUDE_DIR)
  include_directories("${MPI4PY_INCLUDE_DIR}")
else ()
  message(SEND_ERROR
    "Failed to determine the include directory for mpi4py. See "
    "`requirements.txt` in the build directory.")
endif ()

# Needed for mpich 2
add_definitions("-DMPICH_IGNORE_CXX_SEEK")

# FIXME: The ifdefs need to be removed, for now defining it to on
set(${vtk-module}_EXPORT_CODE "#define VTK_USE_MPI")

vtk_module_library(vtkParallelMPI4Py ${Module_SRCS})
vtk_mpi_link(vtkParallelMPI4Py)

set_property(GLOBAL APPEND
  PROPERTY
    vtk_required_python_modules "mpi4py>=2.0.0")
