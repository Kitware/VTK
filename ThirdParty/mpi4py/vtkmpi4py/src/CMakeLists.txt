set(mpi4py_install_dir
  "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}")
set(mpi4py_build_dir
  "${CMAKE_BINARY_DIR}/${mpi4py_install_dir}")

set(python_files
  mpi4py/__init__.py
  mpi4py/__init__.pxd
  mpi4py/MPI.pxd)

if (Python3_VERSION VERSION_GREATER_EQUAL "3.11")
  list(APPEND python_files
    mpi4py/__init__.pyi
    mpi4py/MPI.pyi
    mpi4py/py.typed)
endif ()

set(python_install_files)
foreach (python_file IN LISTS python_files)
  set(output_python_file
    "${mpi4py_build_dir}/${python_file}")
  add_custom_command(
    OUTPUT  "${output_python_file}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/${python_file}"
            "${output_python_file}"
    COMMENT "Copying ${python_file} to the binary directory")
  list(APPEND python_install_files
    "${output_python_file}")
endforeach ()

add_custom_target(vtkmpi4py_python_copy ALL
  DEPENDS
    ${python_install_files})

set(mpi_cfg_content
  "[mpi]
mpicc = ${MPI_C_COMPILER}
mpicxx = ${MPI_CXX_COMPILER}
mpif77 =
mpi490 =\n")
file(GENERATE
  OUTPUT  "${mpi4py_build_dir}/mpi4py/mpi.cfg"
  CONTENT "${mpi_cfg_content}")
list(APPEND python_install_files
  "${mpi4py_build_dir}/mpi4py/mpi.cfg")

install(
  FILES       ${python_install_files}
  DESTINATION "${mpi4py_install_dir}/mpi4py"
  COMPONENT   "python")

if (Python3_SOABI)
  set(_soabi_suffix ".${Python3_SOABI}")
else ()
  set(_soabi_suffix "")
endif ()

add_library(vtkmpi4py.MPI MODULE
  MPI.c)
set_target_properties(vtkmpi4py.MPI
  PROPERTIES
    OUTPUT_NAME "MPI"
    PREFIX      ""
    LIBRARY_OUTPUT_DIRECTORY "${mpi4py_build_dir}/mpi4py")
if (WIN32)
  if ("msmpi" IN_LIST MPI_C_LIB_NAMES)
    if (NOT VTK_MPI4PY_MSMPI_VER)
      # set default msmpi version to 10.0, but also check registry
      set(_msmpi_version_hex 0xa00)
      # XXX(cmake-3.24) use cmake_host_system_information() to read registry
      execute_process(COMMAND REG "QUERY" "HKLM\\SOFTWARE\\Microsoft\\MPI"
        RESULT_VARIABLE _reg_result
        OUTPUT_VARIABLE _reg_output
        ERROR_QUIET)
      if (_reg_result EQUAL 0)
        string(REGEX MATCH "[ \t\r\n]+Version[ \t]+REG_SZ[ \t]+([^ \t\r\n]*)"
          _fullmatch "${_reg_output}")
        set(_msmpi_VERSION "${CMAKE_MATCH_1}")
        string(REGEX MATCH "^([0-9]+)\\.([0-9]+)"
          _short_version "${_msmpi_VERSION}")
        if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.13")
          math(EXPR _msmpi_version_hex
            "${CMAKE_MATCH_1} * 256 + ${CMAKE_MATCH_2}"
            OUTPUT_FORMAT HEXADECIMAL)
        endif ()
      endif ()
      set(VTK_MPI4PY_MSMPI_VER "${_msmpi_version_hex}" CACHE INTERNAL
        "Hexadecimal msmpi version (with 0x) for mpi4py")
    endif ()
    target_compile_definitions(vtkmpi4py.MPI
      PRIVATE
        MSMPI_VER=${VTK_MPI4PY_MSMPI_VER})
  endif ()
  set_target_properties(vtkmpi4py.MPI
    PROPERTIES
      # XXX(python-debug): VTK doesn't expect a debug Python for Debug builds
      # anymore. See vtkModuleWrapPython what actually decides this.
      #DEBUG_POSTFIX "_d"
      SUFFIX "${_soabi_suffix}.pyd")
else ()
  set_target_properties(vtkmpi4py.MPI
    PROPERTIES
      SUFFIX "${_soabi_suffix}${CMAKE_SHARED_MODULE_SUFFIX}")
endif ()

# Assume that fortran bindings are not available.
target_compile_definitions(vtkmpi4py.MPI
  PRIVATE
    PyMPI_MISSING_MPI_Type_create_f90_integer
    PyMPI_MISSING_MPI_Type_create_f90_real
    PyMPI_MISSING_MPI_Type_create_f90_complex
    PyMPI_MISSING_MPI_Status_f2c
    PyMPI_MISSING_MPI_Status_c2f)
target_link_libraries(vtkmpi4py.MPI
  PRIVATE
    VTK::mpi
    VTK::Python)

install(
  TARGETS vtkmpi4py.MPI
  ARCHIVE DESTINATION "${mpi4py_install_dir}/mpi4py"
  LIBRARY DESTINATION "${mpi4py_install_dir}/mpi4py"
  RUNTIME DESTINATION "${mpi4py_install_dir}/mpi4py"
  COMPONENT   "python")

vtk_module_install_headers(
  DIRECTORIES "mpi4py/include/mpi4py/"
  SUBDIR      "vtkmpi4py/mpi4py")
