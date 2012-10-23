cmake_minimum_required(VERSION 2.8)

project(xdmf2)

#options
set(XDMF_USE_ANSI_STDLIB 1)
set(CMAKE_C_FLAGS "${CMAKE_ANSI_CFLAGS} ${CMAKE_C_FLAGS}")

#hard dependencies
#XML
if (XDMF_BUILT_INVTK)
  set(XDMF_LIBXML2_LIBRARIES ${vtklibxml2_LIBRARIES})
  set(XDMF_LIBXML2_INCLUDE_DIRS ${vtklibxml2_INCLUDE_DIRS})
else()
  find_package(LibXml2 REQUIRED)
  set(XDMF_LIBXML2_LIBRARIES ${LIBXML2_LIBRARIES})
  set(XDMF_LIBXML2_INCLUDE_DIRS ${LIBXML2_INCLUDE_DIR})
endif()
#?
if(NOT BUILD_SHARED_LIBS)
  add_definitions(-DLIBXML_STATIC)
endif()

#HDF5
if (XDMF_BUILT_INVTK)
  set(XDMF_HDF5_LIBRARIES ${vtkhdf5_LIBRARIES})
  set(XDMF_HDF5_INCLUDE_DIRS ${vtkhdf5_INCLUDE_DIRS})
else()
  find_package(HDF5 REQUIRED)
  set(XDMF_HDF5_LIBRARIES ${HDF5_LIBRARIES})
  set(XDMF_HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIRS})
endif()
if(EXISTS "${HDF5_CONFIG}")
  include("${HDF5_CONFIG}")
  add_definitions(${HDF5_EXTRA_FLAGS})
endif()

#optional dependencies
#MPI
option(XDMF_BUILD_MPI "XDMF should use MPI" OFF)
mark_as_advanced(XDMF_BUILD_MPI)
if (XDMF_BUILD_MPI)
  find_package(MPI REQUIRED)
  include_directories(${MPI_C_INCLUDE_PATH})
  set(XDMF_MPI_LIBRARIES ${MPI_C_LIBRARIES})
  if (MPI_CXX_LIBRARIES)
    set (XDMF_MPI_LIBRARIES ${XDMF_MPI_LIBRARIES} ${MPI_CXX_LIBRARIES})
  endif()
  add_definitions(-DMPICH_IGNORE_CXX_SEEK)
else()
  add_definitions(-DUSE_SERIAL_XMDF)
endif()

#NDGM
option(XDMF_HAS_NDGM "XDMF has Network Distributed Global Memory (NDGM)" OFF)
mark_as_advanced(XDMF_HAS_NDGM)
if(XDMF_HAS_NDGM)
  option(XDMF_HAS_NDGM_SOURCE "XDMF has Network Distributed Global Memory (NDGM) Source" OFF)
  if(XDMF_HAS_NDGM_SOURCE)
    find_path(NDGM_DIR BuildNDGM.cmake ${xdmf2_SOURCE_DIR}/../Ndgm "Root of Source for NDGM")
    if(NDGM_DIR)
      message(STATUS "......Process NDGM Source")
      include(${NDGM_DIR}/BuildNDGM.cmake)
      #subdirs(NDGM)
      include_directories(${NDGM_DIR}/libsrc)
      set(NDGM_LIBRARY ndgm)
    endif()
  else()
    message(STATUS "......Looking for NDGM_INCLUDE_DIR")
    find_path(NDGM_INCLUDE_DIR Ndgm ${xdmf2_SOURCE_DIR}/../Ndgm/libsrc "Include Directory for NDGM; Resolves #include <Ndgm/ndgm.h>")
    message(STATUS "......Looking for NDGM_LIBRARY")
    find_library(NDGM_LIBRARY ndgm ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
    include_directories(${NDGM_INCLUDE_DIR})
    set(NDGM_LIBRARY ${NDGM_LIBRARY})
  endif()
  set(HAVE_NDGM 1)
endif()

##########################
#packaging
if(NOT XDMF_INSTALL_LIB_DIR)
  set(XDMF_INSTALL_LIB_DIR /lib)
endif()
if(NOT XDMF_INSTALL_INCLUDE_DIR)
  set(XDMF_INSTALL_INCLUDE_DIR /include/Xdmf)
endif()

set(XDMF2_OUTPUT_PATH ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR})
configure_file(${xdmf2_SOURCE_DIR}/XDMFConfig.cmake.in
  ${xdmf2_BINARY_DIR}/XDMFConfig.cmake @ONLY IMMEDIATE)

#setup build
include_directories(
  ${xdmf2_SOURCE_DIR}/libsrc
  ${xdmf2_BINARY_DIR}/libsrc
  ${xdmf2_SOURCE_DIR}/libsrc/utils
  ${XDMF_EXPAT_INCLUDE_DIRS}
  ${XDMF_LIBXML2_INCLUDE_DIRS}
  ${XDMF_HDF5_INCLUDE_DIRS}
  )

include(TestBigEndian)
test_big_endian(CMAKE_WORDS_BIGENDIAN)

add_subdirectory(libsrc)
