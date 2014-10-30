project(XdmfDSM)
cmake_minimum_required(VERSION 2.6)
if (POLICY CMP0015)
  cmake_policy(SET CMP0015 NEW)
endif ()

set(XdmfDSMLinkLibraries XdmfCore)
set(XdmfDSMLinkLibraryDir )

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

option(XDMF_BUILD_DSM_THREADS ON)

mark_as_advanced(Boost_INCLUDE_DIR)
include_directories(${Boost_INCLUDE_DIRS})
mark_as_advanced(FORCE Boost_INCLUDE_DIR)

#mark_as_advanced(CLEAR HDF5_C_INCLUDE_DIR)
#mark_as_advanced(CLEAR HDF5_hdf5_LIBRARY_DEBUG)
#mark_as_advanced(CLEAR HDF5_hdf5_LIBRARY_RELEASE)
#find_package(HDF5 REQUIRED)
if(HDF5_FOUND)
  mark_as_advanced(FORCE HDF5_C_INCLUDE_DIR)
  mark_as_advanced(FORCE HDF5_hdf5_LIBRARY_DEBUG)
  mark_as_advanced(FORCE HDF5_hdf5_LIBRARY_RELEASE)
  include_directories(${HDF5_INCLUDE_DIRS})
  include_directories(${HDF5_C_INCLUDE_DIR})
  set(HDF5_LIBRARIES ${HDF5_hdf5_LIBRARY_RELEASE})
  get_filename_component(HDF5_LIBRARY_DIR "${HDF5_hdf5_LIBRARY_RELEASE}" PATH)
  set(XdmfDSMLinkLibraryDir ${XdmfDSMLinkLibraryDir} ${HDF5_LIBRARY_DIR})
  # FIXME: Would like to get this info from HDF5 so we don't have conflicting
  # MPI versions
  if(HDF5_IS_PARALLEL)
    find_package(MPI REQUIRED)
    if(MPI_FOUND)
      include_directories(${MPI_INCLUDE_PATH})
      set(XdmfDSMLinkLibraries ${XdmfDSMLinkLibraries} ${MPI_LIBRARY} ${MPI_EXTRA_LIBRARY})
    endif()
  else()
    message(SEND_ERROR
      "Error: XdmfDSM requires a parallel enabled build of HDF5.")
  endif()
  set(XdmfDSMLinkLibraries ${XdmfDSMLinkLibraries} ${HDF5_LIBRARIES})
endif()


find_package(LibXml2 REQUIRED)
if(LIBXML2_FOUND)
  include_directories(${LIBXML2_INCLUDE_DIR})
  set(XdmfDSMLinkLibraries ${XdmfDSMLinkLibraries} ${LIBXML2_LIBRARIES})
endif()

set(XdmfDSMSources
  XdmfHDF5ControllerDSM
  XdmfHDF5WriterDSM
  XdmfDSMCommMPI
  XdmfDSMBuffer
  XdmfDSMManager
  XdmfDSMDriver)

if (XDMF_BUILD_DSM_THREADS)
  add_definitions(-DXDMF_BUILD_DSM_THREADS)
endif ()


if (XDMF_BUILD_DSM_THREADS)
  find_package(H5FDdsm REQUIRED NO_MODULE)
  if(H5FDdsm_FOUND)
    include_directories(${H5FDdsm_INCLUDE_DIR})
    get_property(h5fddsm_lib_location TARGET H5FDdsm PROPERTY LOCATION)
    set(H5FDdsm_LIBRARIES ${h5fddsm_lib_location})
    set(XdmfDSMLinkLibraries ${XdmfDSMLinkLibraries} ${H5FDdsm_LIBRARIES})
  else()
    message(SEND_ERROR
      "Cannot find HDF5 DSM!  Please set H5FDdsm_DIR or disable XDMF_BUILD_DSM_THREADS and configure again.")
  endif()
endif ()

add_library(XdmfDSM ${XdmfDSMSources})
link_directories(${XDMF_LIBRARY_DIRS} ${XdmfDSMLinkLibraryDir})
target_link_libraries(XdmfDSM ${XdmfDSMLinkLibraries})

if(WIN32)
  add_definitions(-D_HDF5USEDLL_ -D_HDF5USEHLDLL_)
  set_target_properties(XdmfDSM PROPERTIES
      DEFINE_SYMBOL XdmfDSM_EXPORTS)
  if(NOT MSVC10)
        set_target_properties(XdmfDSM PROPERTIES
      PREFIX ../
      IMPORT_PREFIX ../
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
  endif()
endif()

if(XDMF_WRAP_JAVA)
  XDMF_SWIG_JAVA(XdmfDSM Compiled_XdmfCore_Jar)
endif()

if(XDMF_WRAP_PYTHON)
  XDMF_SWIG_PYTHON(XdmfDSM XdmfCore)
endif()


set(XDMF_LIBRARY_DIRS ${XDMF_LIBRARY_DIRS} PARENT_SCOPE)

if(WIN32)
    set(XDMFDSM_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/XdmfDSM.lib PARENT_SCOPE)
endif()

if(UNIX)
    set(XDMFDSM_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/libXdmfDSM.so
        PARENT_SCOPE)
endif()

if(APPLE)
    set(XDMFDSM_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/libXdmfDSM.dylib
        PARENT_SCOPE)
endif()

file(GLOB_RECURSE
  XdmfDSMHeaders
  "*.hpp" "*.tpp" "*.i"
  "../../CMake/VersionSuite/*.hpp")
install(FILES ${XdmfDSMHeaders} DESTINATION include)
install(TARGETS XdmfDSM
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

if(XDMF_BUILD_DSM_THREADS)
set(XdmfDSM_INCLUDE_DIRS
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${HDF5_INCLUDE_DIR}
  ${H5FDdsm_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${PYTHON_INCLUDE_DIRS}
  CACHE INTERNAL "")
else()
set(XdmfDSM_INCLUDE_DIRS
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${HDF5_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${PYTHON_INCLUDE_DIRS}
  CACHE INTERNAL "")
endif()

include_directories(${XdmfCore_INCLUDE_DIRS} ${XdmfDSM_INCLUDE_DIRS})

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
