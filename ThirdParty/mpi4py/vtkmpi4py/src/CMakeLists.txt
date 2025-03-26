set(mpi4py_install_dir
  "${VTK_PYTHON_SITE_PACKAGES_SUFFIX}")
set(mpi4py_build_dir
  "${CMAKE_BINARY_DIR}/${mpi4py_install_dir}")

set(python_files
  mpi4py/__init__.py
  mpi4py/__init__.pxd
  mpi4py/MPI.pxd)

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

add_library(vtkmpi4py.MPI MODULE
  MPI.c)
set_target_properties(vtkmpi4py.MPI
  PROPERTIES
    OUTPUT_NAME "MPI"
    PREFIX      ""
    LIBRARY_OUTPUT_DIRECTORY "${mpi4py_build_dir}/mpi4py")
if (WIN32)
  set_target_properties(vtkmpi4py.MPI
    PROPERTIES
      # XXX(python-debug): VTK doesn't expect a debug Python for Debug builds
      # anymore. See vtkModuleWrapPython what actually decides this.
      #DEBUG_POSTFIX "_d"
      SUFFIX        ".pyd")
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
