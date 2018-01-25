if (WIN32)
  message(STATUS "Not building mpi4py on Windows (not supported currently).")
  return()
endif()

# Author:  Lisandro Dalcin
# Contact: dalcinl@gmail.com
# ** Note this file has been modified from the original to work with ParaView **
cmake_minimum_required(VERSION 2.6)
project(mpi4py)

#------------------------------------------------------------------------------
# Set locations where to install the binaries.

# MPI4PY_INSTALL_PACKAGE_DIR is the location where all mpi4py package is
# installed.
# are to be installed.
if (NOT MPI4PY_INSTALL_PACKAGE_DIR)
  set (MPI4PY_INSTALL_PACKAGE_DIR "lib/site-packages")
endif()

if (NOT MPI4PY_PACKAGE_BINARY_DIR)
  set (MPI4PY_PACKAGE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

find_package(PythonInterp ${VTK_PYTHON_VERSION} REQUIRED)
if (VTK_UNDEFINED_SYMBOLS_ALLOWED)
  set(_QUIET_LIBRARY "QUIET")
else()
  set(_QUIET_LIBRARY "${VTK_PYTHON_VERSION}" "REQUIRED")
endif()
find_package(PythonLibs ${_QUIET_LIBRARY})
find_package(MPI REQUIRED)

# -------------------------------------------------------------------------
# For mpich which is not build with fortran some symbols are not found at
# link time. The following solution prevents this.
check_function_exists(MPI_Type_create_f90_integer f90_integer_defined)
if (NOT f90_integer_defined)
  # Assume the other MPI_Type_create_f90_* functions are unavailable
  add_definitions(-DPyMPI_MISSING_MPI_Type_create_f90_integer
                  -DPyMPI_MISSING_MPI_Type_create_f90_real
                  -DPyMPI_MISSING_MPI_Type_create_f90_complex)
endif()

check_function_exists(MPI_Status_fc2 f2c_defined)
if (NOT f2c_defined)
  # Assume MPI_Status_c2f is not defined either
  add_definitions(-DPyMPI_MISSING_MPI_Status_f2c
                  -DPyMPI_MISSING_MPI_Status_c2f)
endif()

# -----------------------------------------------------------------------------
# Note: In ParaView FindPythonLibs.cmake module is additionally defined in VTK
# and overides the default module from CMake. As a consequence PYTHON_ADD_MODULE
# needs to be defined here.

# PYTHON_ADD_MODULE(<name> src1 src2 ... srcN) is used to build modules for python.
# PYTHON_WRITE_MODULES_HEADER(<filename>) writes a header file you can include
# in your sources to initialize the static python modules
function(PYTHON_ADD_MODULE _NAME )
  get_property(_TARGET_SUPPORTS_SHARED_LIBS
    GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
  option(PYTHON_ENABLE_MODULE_${_NAME} "Add module ${_NAME}" TRUE)
  option(PYTHON_MODULE_${_NAME}_BUILD_SHARED
    "Add module ${_NAME} shared" ${_TARGET_SUPPORTS_SHARED_LIBS})

  # Mark these options as advanced
  mark_as_advanced(PYTHON_ENABLE_MODULE_${_NAME}
    PYTHON_MODULE_${_NAME}_BUILD_SHARED)

  if(PYTHON_ENABLE_MODULE_${_NAME})
    if(PYTHON_MODULE_${_NAME}_BUILD_SHARED)
      set(PY_MODULE_TYPE MODULE)
    else()
      set(PY_MODULE_TYPE STATIC)
      set_property(GLOBAL  APPEND  PROPERTY  PY_STATIC_MODULES_LIST ${_NAME})
    endif()

    set_property(GLOBAL  APPEND  PROPERTY  PY_MODULES_LIST ${_NAME})
    add_library(${_NAME} ${PY_MODULE_TYPE} ${ARGN})
    #    target_link_libraries(${_NAME} ${PYTHON_LIBRARIES})

    if(PYTHON_MODULE_${_NAME}_BUILD_SHARED)
      set_target_properties(${_NAME} PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}")
      if(WIN32 AND NOT CYGWIN)
        set_target_properties(${_NAME} PROPERTIES SUFFIX ".pyd")
      endif()
    endif()

  endif()
endfunction()

# -----------------------------------------------------------------------------
set(mpi4py_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(mpi4py_BINARY_DIR "${MPI4PY_PACKAGE_BINARY_DIR}/mpi4py")

file(GLOB mpi4py_PYTHON_FILES
  RELATIVE ${mpi4py_SOURCE_DIR}
  ${mpi4py_SOURCE_DIR}/*.py)

file(GLOB mpi4py_HEADER_FILES
  RELATIVE ${mpi4py_SOURCE_DIR}
  ${mpi4py_SOURCE_DIR}/include/mpi4py/*.px[di]
  ${mpi4py_SOURCE_DIR}/include/mpi4py/*.pyx
  ${mpi4py_SOURCE_DIR}/include/mpi4py/*.[hi]
  )

foreach(file
    ${mpi4py_PYTHON_FILES}
    ${mpi4py_HEADER_FILES}
    )
  set(src "${mpi4py_SOURCE_DIR}/${file}")
  set(tgt "${mpi4py_BINARY_DIR}/${file}")
  add_custom_command(
    DEPENDS ${src}
    OUTPUT  ${tgt}
    COMMAND ${CMAKE_COMMAND} ARGS -E copy ${src} ${tgt}
    COMMENT "copy: ${file}"
    )
  set(mpi4py_OUTPUT_FILES ${mpi4py_OUTPUT_FILES} ${tgt})
endforeach()

foreach(file ${mpi4py_PYTHON_FILES})
  set(mpi4py_py  ${mpi4py_py}  "${mpi4py_BINARY_DIR}/${file}")
  set(mpi4py_pyc ${mpi4py_pyc} "${mpi4py_BINARY_DIR}/${file}c")
  set(mpi4py_pyo ${mpi4py_pyo} "${mpi4py_BINARY_DIR}/${file}o")
endforeach()
set(CMAKE_CONFIGURABLE_FILE_CONTENT
    "from compileall import compile_dir\ncompile_dir(\"${mpi4py_BINARY_DIR}\")")
configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
               "${CMAKE_CURRENT_BINARY_DIR}/compile_py" @ONLY)
unset(CMAKE_CONFIGURABLE_FILE_CONTENT)
add_custom_command(
  COMMAND ${PYTHON_EXECUTABLE} ARGS compile_py
  COMMAND ${PYTHON_EXECUTABLE} ARGS -O compile_py
  DEPENDS ${mpi4py_py} ${CMAKE_CURRENT_BINARY_DIR}/compile_py
  OUTPUT  ${mpi4py_pyc} ${mpi4py_pyo}
  )
set(mpi4py_OUTPUT_FILES ${mpi4py_OUTPUT_FILES} ${mpi4py_pyc} ${mpi4py_pyo})
set (mpicc)
set (mpicxx)
set (mpif77)
set (mpif90)
if (MPI_C_FOUND)
  set (mpicc ${MPI_C_COMPILER})
endif()
if (MPI_CXX_FOUND)
  set (mpicxx ${MPI_CXX_COMPILER})
endif()
# NOTE: We are ignoring the fortran components for now.
foreach(file "mpi.cfg")
  set(tgt "${mpi4py_BINARY_DIR}/${file}")
  set(CMAKE_CONFIGURABLE_FILE_CONTENT "[mpi]
mpicc  = ${mpicc}
mpicxx = ${mpicxx}
mpif77 = ${mpif77}
mpif90 = ${mpif90}")
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                 "${tgt}" @ONLY)
  unset(CMAKE_CONFIGURABLE_FILE_CONTENT)
  set(mpi4py_OUTPUT_FILES ${mpi4py_OUTPUT_FILES} ${tgt})
endforeach()

add_custom_target(mpi4py ALL DEPENDS ${mpi4py_OUTPUT_FILES})

include_directories(
  ${MPI_C_INCLUDE_PATH}
  ${PYTHON_INCLUDE_DIRS}
  "${mpi4py_SOURCE_DIR}"
  )

# --- mpi4py.MPI ---
PYTHON_ADD_MODULE(mpi4py.MPI MODULE "${mpi4py_SOURCE_DIR}/MPI.c")
set_target_properties(
  mpi4py.MPI PROPERTIES
  OUTPUT_NAME "MPI" PREFIX ""
  COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}"
  LINK_FLAGS "${MPI_C_LINK_FLAGS}"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}"
  LINKER_LANGUAGE C
  )
target_link_libraries(mpi4py.MPI ${PYTHON_LIBRARY})
target_link_libraries(mpi4py.MPI ${MPI_C_LIBRARIES})
target_link_libraries(mpi4py.MPI ${CMAKE_DL_LIBS})

# --- mpi4py.dl ---
PYTHON_ADD_MODULE(mpi4py.dl MODULE "${mpi4py_SOURCE_DIR}/dynload.c")
set_target_properties(
  mpi4py.dl PROPERTIES
  OUTPUT_NAME "dl" PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}"
  LINKER_LANGUAGE C
  )
target_link_libraries(mpi4py.dl ${PYTHON_LIBRARY})
target_link_libraries(mpi4py.dl ${CMAKE_DL_LIBS})

# --- mpi4py/lib-pmpi/libmpe.so ---
add_library(pmpi-mpe MODULE "${mpi4py_SOURCE_DIR}/lib-pmpi/mpe.c")
set_target_properties(
  pmpi-mpe PROPERTIES
  OUTPUT_NAME "mpe"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  LINKER_LANGUAGE C
  INCLUDE_DIRECTORIES "${mpi4py_SOURCE_DIR}/lib-pmpi"
  )
target_link_libraries(pmpi-mpe ${MPE_LIBRARIES})
target_link_libraries(pmpi-mpe ${MPI_C_LIBRARIES})

# --- mpi4py/lib-pmpi/libvt.so ---
add_library(pmpi-vt MODULE "${mpi4py_SOURCE_DIR}/lib-pmpi/vt.c")
set_target_properties(
  pmpi-vt PROPERTIES
  OUTPUT_NAME "vt"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  LINKER_LANGUAGE C
  INCLUDE_DIRECTORIES "${mpi4py_SOURCE_DIR}/lib-pmpi"
  )
target_link_libraries(pmpi-vt ${VT_LIBRARIES})
target_link_libraries(pmpi-vt ${MPI_C_LIBRARIES})

# --- mpi4py/lib-pmpi/libvt-mpi.so ---
add_library(pmpi-vt-mpi MODULE "${mpi4py_SOURCE_DIR}/lib-pmpi/vt-mpi.c")
set_target_properties(
  pmpi-vt-mpi PROPERTIES
  OUTPUT_NAME "vt-mpi"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  LINKER_LANGUAGE C
  INCLUDE_DIRECTORIES "${mpi4py_SOURCE_DIR}/lib-pmpi"
  )
target_link_libraries(pmpi-vt-mpi ${VT_MPI_LIBRARIES})
target_link_libraries(pmpi-vt-mpi ${MPI_C_LIBRARIES})

# --- mpi4py/lib-pmpi/libvt-hyb.so ---
add_library(pmpi-vt-hyb MODULE "${mpi4py_SOURCE_DIR}/lib-pmpi/vt-hyb.c")
set_target_properties(
  pmpi-vt-hyb PROPERTIES
  OUTPUT_NAME "vt-hyb"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  LINKER_LANGUAGE C
  INCLUDE_DIRECTORIES "${mpi4py_SOURCE_DIR}/lib-pmpi"
  )
target_link_libraries(pmpi-vt-hyb ${VT_HYB_LIBRARIES})
target_link_libraries(pmpi-vt-hyb ${MPI_C_LIBRARIES})

# -----------------------------------------------------------------------------
# Install the paraview module files.
install(DIRECTORY ${mpi4py_BINARY_DIR}
  DESTINATION ${MPI4PY_INSTALL_PACKAGE_DIR}
  COMPONENT Runtime)
