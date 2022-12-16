include(CMakeFindDependencyMacro)
find_dependency("Python3" COMPONENTS Interpreter)
execute_process(
  COMMAND "${Python3_EXECUTABLE}" -c
          "import mpi4py"
  OUTPUT_VARIABLE _mpi4py_out
  ERROR_VARIABLE  _mpi4py_out
  RESULT_VARIABLE _mpi4py_res)
if (_mpi4py_res)
  set(mpi4py_FOUND 0)
endif ()

if (NOT mpi4py_INCLUDE_DIR OR NOT EXISTS "${mpi4py_INCLUDE_DIR}")
  execute_process(
    COMMAND "${Python3_EXECUTABLE}" -c
            "import mpi4py; print(mpi4py.get_include())"
    OUTPUT_VARIABLE _mpi4py_INCLUDE_DIR
    RESULT_VARIABLE _mpi4py_include_dir_res
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (_mpi4py_include_dir_res)
    set(mpi4py_FOUND 0)
  endif ()
  set(mpi4py_INCLUDE_DIR "${_mpi4py_INCLUDE_DIR}"
    CACHE INTERNAL "Path of the MPI4Py include directory")
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(mpi4py
  REQUIRED_VARS mpi4py_INCLUDE_DIR)

if (mpi4py_FOUND AND NOT TARGET mpi4py::mpi4py)
  add_library(mpi4py::mpi4py INTERFACE IMPORTED)
  set_target_properties(mpi4py::mpi4py
    PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${mpi4py_INCLUDE_DIR}")
endif ()
