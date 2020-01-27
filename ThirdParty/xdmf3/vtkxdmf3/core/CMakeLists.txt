project(XdmfCore)

include(CheckCXXSourceCompiles)
include(SetUpVersion)
include(TestBigEndian)

if(VERSION_CONTROL_AUTOUPDATE OR
    NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/XdmfVersion.hpp)
  VersionCreate(
    "Xdmf"
    "${XDMF_MAJOR_VERSION}" "${XDMF_MINOR_VERSION}" "${XDMF_PATCH_VERSION}"
    "XDMFCORE_EXPORT" "XdmfCore.hpp"
    )
endif()

if(BUILD_SHARED_LIBS)
  set(BUILD_SHARED 1)
endif()

if (FALSE) # XXX(kitware): use target-based include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif ()

if (FALSE) # XXX(kitware): VTK's module system handles dependencies
find_package(Boost REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(HDF5 REQUIRED)
if(HDF5_FOUND)
  include_directories(${HDF5_INCLUDE_DIRS})
  include_directories(${HDF5_C_INCLUDE_DIR})
  # FIXME: Would like to get this info from HDF5 so we don't have conflicting
  # MPI versions
  if(HDF5_IS_PARALLEL)
    if (NOT MPI_FOUND)
      find_package(MPI REQUIRED)
      if(MPI_FOUND)
        include_directories(${MPI_INCLUDE_PATH})
      endif()
    endif ()
  endif()
endif()


find_package(LibXml2 REQUIRED)
if(LIBXML2_FOUND)
  include_directories(${LIBXML2_INCLUDE_DIR})
endif()
else ()
vtk_module_find_package(
  PACKAGE Boost
  FORWARD_VERSION_REQ EXACT
  VERSION_VAR "@Boost_MAJOR_VERSION@.@Boost_MINOR_VERSION@.@Boost_SUBMINOR_VERSION@")
endif ()

# Perform compile-time checks and generate XdmfCoreConfig.hpp

TEST_BIG_ENDIAN(XDMF_BIG_ENDIAN)

set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES} ${Boost_INCLUDE_DIRS})
check_cxx_source_compiles("
#include <boost/shared_ptr.hpp>

struct Base { virtual ~Base(){} };
struct Derived : public Base {};

int main(int ac, char * av[])
{
  boost::shared_ptr<Base> ptr(new Base());
  boost::shared_dynamic_cast<Derived>(ptr);
}
"
HAVE_BOOST_SHARED_DYNAMIC_CAST)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/XdmfCoreConfig.hpp.in
               ${CMAKE_CURRENT_BINARY_DIR}/XdmfCoreConfig.hpp)

set(XdmfCoreSources
  XdmfArray
  XdmfArrayReference
  XdmfArrayType
  XdmfBinaryController
  XdmfCoreItemFactory
  XdmfCoreReader
  XdmfError
  XdmfFunction
  XdmfHDF5Controller
  XdmfHDF5Writer
  XdmfHeavyDataController
  XdmfHeavyDataDescription
  XdmfHeavyDataWriter
  XdmfInformation
  XdmfItem
  XdmfItemProperty
  XdmfPlaceholder
  XdmfSparseMatrix
  XdmfSubset
  XdmfSystemUtils
  ${CMAKE_CURRENT_BINARY_DIR}/XdmfVersion
  XdmfVisitor
  XdmfWriter)

if (TIFF_FOUND)
  set(XdmfCoreSources
      ${XdmfCoreSources}
      XdmfTIFFController)
  set(FOUND_TIFF_LOCATION core/XdmfTIFFController)
else ()
  set(FOUND_TIFF_LOCATION "")
endif()

if (FALSE) # XXX(kitware): VTK's module system handles dependencies
set(XdmfCoreSources
  core/XdmfArray
  core/XdmfArrayReference
  core/XdmfArrayType
  core/XdmfBinaryController
  core/XdmfCoreItemFactory
  core/XdmfCoreReader
  core/XdmfError
  core/XdmfFunction
  core/XdmfHDF5Controller
  core/XdmfHDF5Writer
  core/XdmfHeavyDataController
  core/XdmfHeavyDataDescription
  core/XdmfHeavyDataWriter
  core/XdmfInformation
  core/XdmfItem
  core/XdmfItemProperty
  core/XdmfPlaceholder
  core/XdmfSparseMatrix
  core/XdmfSubset
  core/XdmfSystemUtils
  ${FOUND_TIFF_LOCATION}
  ${CMAKE_CURRENT_BINARY_DIR}/XdmfVersion
  core/XdmfVisitor
  core/XdmfWriter
  PARENT_SCOPE)
endif ()

if (FALSE) # XXX(kitware): use VTK's module system.
add_library(XdmfCore ${XdmfCoreSources})
SET_TARGET_PROPERTIES(
  XdmfCore PROPERTIES
  VERSION ${XDMF_VERSION}
  SOVERSION ${XDMF_MAJOR_VERSION}
  )

target_link_libraries(XdmfCore
  PUBLIC
    ${HDF5_C_LIBRARIES}
    ${LIBXML2_LIBRARIES})
if (TIFF_FOUND)
  target_link_libraries(XdmfCore
    PRIVATE
      ${TIFF_LIBRARIES})
endif ()
else ()
set(sources)
foreach (xdmf_core_source IN LISTS XdmfCoreSources)
  list(APPEND sources
    "${xdmf_core_source}.cpp")
endforeach ()

add_library(vtkxdmfcore ${sources})
target_include_directories(vtkxdmfcore
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../CMake/VersionSuite>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtkxdmf3/core>")
target_link_libraries(vtkxdmfcore
  PUBLIC
    VTK::libxml2
    Boost::boost
  PRIVATE
    VTK::hdf5)
_vtk_module_apply_properties(vtkxdmfcore)
_vtk_module_install(vtkxdmfcore)
endif ()

if(WIN32)
  #add_definitions(-D_HDF5USEDLL_ -D_HDF5USEHLDLL_)
  if (BUILD_SHARED_LIBS)
    # XXX(kitware): mangle the library name.
    set_target_properties(vtkxdmfcore PROPERTIES
      DEFINE_SYMBOL XdmfCore_EXPORTS)
  endif()
endif()

if(XDMF_WRAP_JAVA)
  XDMF_SWIG_JAVA(XdmfCore)
endif()

if(XDMF_WRAP_PYTHON)
  if (NOT BUILD_SHARED_LIBS)
    message(FATAL_ERROR "Python Wrappers do not function"
                        " properly without shared libraries")
  endif (NOT BUILD_SHARED_LIBS)
  XDMF_SWIG_PYTHON(XdmfCore)
endif()

file(GLOB_RECURSE
  XdmfCoreHeaders
  "*.hpp" "*.tpp" "*.i"
  "../CMake/VersionSuite/*.hpp")
file(GLOB LokiHeaders loki/*.h)

set(XdmfCoreHeaders
      ${XdmfCoreHeaders}
      ${CMAKE_CURRENT_BINARY_DIR}/XdmfCoreConfig.hpp)
if (FALSE)
install(FILES ${XdmfCoreHeaders} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${LokiHeaders} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/loki)
install(TARGETS XdmfCore
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
else ()
vtk_module_install_headers(
  FILES   ${XdmfCoreHeaders}
  SUBDIR  "vtkxdmf3/core")
vtk_module_install_headers(
  FILES   ${LokiHeaders}
  SUBDIR  "vtkxdmf3/core/loki")
endif ()

if (Boost_FOUND)
  set(FOUND_BOOST_INCLUDES ${Boost_INCLUDE_DIRS})
else ()
  set(FOUND_BOOST_INCLUDES "")
endif ()
set(XdmfCore_INCLUDE_DIRS
  ${FOUND_BOOST_INCLUDES}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${HDF5_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${PYTHON_INCLUDE_DIRS}
  CACHE INTERNAL "")

set(XDMF_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/include PARENT_SCOPE)
set(XDMF_DIR ${CMAKE_INSTALL_PREFIX} PARENT_SCOPE)

if(XDMF_BUILD_DSM)
  add_subdirectory(dsm)
  set(XDMF_DSM_IS_CRAY "${XDMF_DSM_IS_CRAY}" PARENT_SCOPE)
endif(XDMF_BUILD_DSM)

if(XDMF_BUILD_TESTING)
  add_subdirectory(tests)
endif()
