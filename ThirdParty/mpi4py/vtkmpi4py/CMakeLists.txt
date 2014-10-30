if (WIN32)
  message(STATUS "Not building mpi4py on Windows (not supported currently).")
  return()
endif()

# Author:  Lisandro Dalcin
# Contact: dalcinl@gmail.com
# ** Note this file has been modified from the original to work with ParaView **
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(mpi4py)

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

FIND_PACKAGE(PythonInterp ${VTK_PYTHON_VERSION})
FIND_PACKAGE(PythonLibs ${VTK_PYTHON_VERSION})
FIND_PACKAGE(MPI)

# -------------------------------------------------------------------------
# For mpich which is not build with fortran some symbols are not found at
# link time. The following solution prevents this.
add_definitions(-DPyMPI_MISSING_MPI_Type_create_f90_integer
                -DPyMPI_MISSING_MPI_Type_create_f90_real
                -DPyMPI_MISSING_MPI_Type_create_f90_complex)

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
SET(mpi4py_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
SET(mpi4py_BINARY_DIR "${MPI4PY_PACKAGE_BINARY_DIR}/mpi4py")

FILE(GLOB mpi4py_PYTHON_FILES
  RELATIVE ${mpi4py_SOURCE_DIR}
  ${mpi4py_SOURCE_DIR}/*.py)

FILE(GLOB mpi4py_HEADER_FILES
  RELATIVE ${mpi4py_SOURCE_DIR}
  ${mpi4py_SOURCE_DIR}/include/mpi4py/*.px[di]
  ${mpi4py_SOURCE_DIR}/include/mpi4py/*.pyx
  ${mpi4py_SOURCE_DIR}/include/mpi4py/*.[hi]
  )

FOREACH(file
    ${mpi4py_PYTHON_FILES}
    ${mpi4py_HEADER_FILES}
    )
  SET(src "${mpi4py_SOURCE_DIR}/${file}")
  SET(tgt "${mpi4py_BINARY_DIR}/${file}")
  ADD_CUSTOM_COMMAND(
    DEPENDS ${src}
    OUTPUT  ${tgt}
    COMMAND ${CMAKE_COMMAND} ARGS -E copy ${src} ${tgt}
    COMMENT "copy: ${file}"
    )
  SET(mpi4py_OUTPUT_FILES ${mpi4py_OUTPUT_FILES} ${tgt})
ENDFOREACH()

FOREACH(file ${mpi4py_PYTHON_FILES})
  SET(mpi4py_py  ${mpi4py_py}  "${mpi4py_BINARY_DIR}/${file}")
  SET(mpi4py_pyc ${mpi4py_pyc} "${mpi4py_BINARY_DIR}/${file}c")
  SET(mpi4py_pyo ${mpi4py_pyo} "${mpi4py_BINARY_DIR}/${file}o")
ENDFOREACH()
SET(CMAKE_CONFIGURABLE_FILE_CONTENT
    "from compileall import compile_dir\ncompile_dir(\"${mpi4py_BINARY_DIR}\")")
configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
               "${CMAKE_CURRENT_BINARY_DIR}/compile_py" @ONLY)
UNSET(CMAKE_CONFIGURABLE_FILE_CONTENT)
ADD_CUSTOM_COMMAND(
  COMMAND ${PYTHON_EXECUTABLE} ARGS compile_py
  COMMAND ${PYTHON_EXECUTABLE} ARGS -O compile_py
  DEPENDS ${mpi4py_py} ${CMAKE_CURRENT_BINARY_DIR}/compile_py
  OUTPUT  ${mpi4py_pyc} ${mpi4py_pyo}
  )
SET(mpi4py_OUTPUT_FILES ${mpi4py_OUTPUT_FILES} ${mpi4py_pyc} ${mpi4py_pyo})
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
FOREACH(file "mpi.cfg")
  SET(tgt "${mpi4py_BINARY_DIR}/${file}")
  SET(CMAKE_CONFIGURABLE_FILE_CONTENT "[mpi]
mpicc  = ${mpicc}
mpicxx = ${mpicxx}
mpif77 = ${mpif77}
mpif90 = ${mpif90}")
  configure_file(${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in
                 "${tgt}" @ONLY)
  UNSET(CMAKE_CONFIGURABLE_FILE_CONTENT)
  SET(mpi4py_OUTPUT_FILES ${mpi4py_OUTPUT_FILES} ${tgt})
ENDFOREACH()

ADD_CUSTOM_TARGET(mpi4py ALL DEPENDS ${mpi4py_OUTPUT_FILES})

INCLUDE_DIRECTORIES(
  ${MPI_C_INCLUDE_PATH}
  ${PYTHON_INCLUDE_DIRS}
  "${mpi4py_SOURCE_DIR}"
  )

# --- mpi4py.MPI ---
PYTHON_ADD_MODULE(mpi4py.MPI MODULE "${mpi4py_SOURCE_DIR}/MPI.c")
SET_TARGET_PROPERTIES(
  mpi4py.MPI PROPERTIES
  OUTPUT_NAME "MPI" PREFIX ""
  COMPILE_FLAGS "${MPI_C_COMPILE_FLAGS}"
  LINK_FLAGS "${MPI_C_LINK_FLAGS}"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}"
  LINKER_LANGUAGE C
  )
TARGET_LINK_LIBRARIES(mpi4py.MPI ${PYTHON_LIBRARY})
TARGET_LINK_LIBRARIES(mpi4py.MPI ${MPI_C_LIBRARIES})
TARGET_LINK_LIBRARIES(mpi4py.MPI ${CMAKE_DL_LIBS})


# --- mpi4py.MPE ---
PYTHON_ADD_MODULE(mpi4py.MPE MODULE ${mpi4py_SOURCE_DIR}/MPE.c)
SET_TARGET_PROPERTIES(
  mpi4py.MPE PROPERTIES
  OUTPUT_NAME "MPE" PREFIX ""
  COMPILE_FLAGS "${MPE_COMPILE_FLAGS}" "${MPI_C_COMPILE_FLAGS}"
  LINK_FLAGS "${MPE_LINK_FLAGS}" "${MPI_C_LINK_FLAGS}"
  LIBRARY_OUTPUT_DIRECTORY ${mpi4py_BINARY_DIR}
  RUNTIME_OUTPUT_DIRECTORY ${mpi4py_BINARY_DIR}
  LINKER_LANGUAGE C
  )
TARGET_LINK_LIBRARIES(mpi4py.MPE ${PYTHON_LIBRARY})
TARGET_LINK_LIBRARIES(mpi4py.MPE ${MPE_LIBRARY})
TARGET_LINK_LIBRARIES(mpi4py.MPE ${MPI_C_LIBRARIES})


# --- mpi4py.dl ---
PYTHON_ADD_MODULE(mpi4py.dl MODULE "${mpi4py_SOURCE_DIR}/dynload.c")
SET_TARGET_PROPERTIES(
  mpi4py.dl PROPERTIES
  OUTPUT_NAME "dl" PREFIX ""
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}"
  LINKER_LANGUAGE C
  )
TARGET_LINK_LIBRARIES(mpi4py.dl ${PYTHON_LIBRARY})
TARGET_LINK_LIBRARIES(mpi4py.dl ${CMAKE_DL_LIBS})

# --- mpi4py/lib-pmpi/libmpe.so ---
ADD_LIBRARY(pmpi-mpe MODULE "${mpi4py_SOURCE_DIR}/pmpi-mpe.c")
SET_TARGET_PROPERTIES(
  pmpi-mpe PROPERTIES
  OUTPUT_NAME "mpe"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  LINKER_LANGUAGE C
  )
TARGET_LINK_LIBRARIES(pmpi-mpe ${MPE_LIBRARIES})
TARGET_LINK_LIBRARIES(pmpi-mpe ${MPI_C_LIBRARIES})

# --- mpi4py/lib-pmpi/libvt.so ---
ADD_LIBRARY(pmpi-vt MODULE "${mpi4py_SOURCE_DIR}/pmpi-vt.c")
SET_TARGET_PROPERTIES(
  pmpi-vt PROPERTIES
  OUTPUT_NAME "vt"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  LINKER_LANGUAGE C
  )
TARGET_LINK_LIBRARIES(pmpi-vt ${VT_LIBRARIES})
TARGET_LINK_LIBRARIES(pmpi-vt ${MPI_C_LIBRARIES})

# --- mpi4py/lib-pmpi/libvt-mpi.so ---
ADD_LIBRARY(pmpi-vt-mpi MODULE "${mpi4py_SOURCE_DIR}/pmpi-vt-mpi.c")
SET_TARGET_PROPERTIES(
  pmpi-vt-mpi PROPERTIES
  OUTPUT_NAME "vt-mpi"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  LINKER_LANGUAGE C
  )
TARGET_LINK_LIBRARIES(pmpi-vt-mpi ${VT_MPI_LIBRARIES})
TARGET_LINK_LIBRARIES(pmpi-vt-mpi ${MPI_C_LIBRARIES})

# --- mpi4py/lib-pmpi/libvt-hyb.so ---
ADD_LIBRARY(pmpi-vt-hyb MODULE "${mpi4py_SOURCE_DIR}/pmpi-vt-hyb.c")
SET_TARGET_PROPERTIES(
  pmpi-vt-hyb PROPERTIES
  OUTPUT_NAME "vt-hyb"
  LIBRARY_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  RUNTIME_OUTPUT_DIRECTORY "${mpi4py_BINARY_DIR}/lib-pmpi"
  LINKER_LANGUAGE C
  )
TARGET_LINK_LIBRARIES(pmpi-vt-hyb ${VT_HYB_LIBRARIES})
TARGET_LINK_LIBRARIES(pmpi-vt-hyb ${MPI_C_LIBRARIES})

# -----------------------------------------------------------------------------
# Install the paraview module files.
INSTALL(DIRECTORY ${mpi4py_BINARY_DIR}
  DESTINATION ${MPI4PY_INSTALL_PACKAGE_DIR}
  COMPONENT Runtime)
