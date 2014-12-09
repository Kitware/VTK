project(XdmfCore)
cmake_minimum_required(VERSION 2.6)
if (POLICY CMP0015)
  cmake_policy(SET CMP0015 NEW)
endif ()

include(CheckCXXSourceCompiles)
include(SetUpVersion)
include(TestBigEndian)

if(VERSION_CONTROL_AUTOUPDATE OR 
    NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/XdmfVersion.hpp)
  VersionCreate("Xdmf" "2" "XDMFCORE_EXPORT" "XdmfCore.hpp")
endif()

if(BUILD_SHARED_LIBS)
  set(LIBTYPE SHARED)
  set(BUILD_SHARED 1)
else()
  set(LIBTYPE STATIC)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
#set(CMAKE_SKIP_BUILD_RPATH  FALSE)
#set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
#set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)
#set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

option(XDMF_BUILD_DSM OFF)
mark_as_advanced(XDMF_BUILD_DSM)

if(XDMF_BUILD_DSM)
  option(XDMF_BUILD_DSM_THREADS ON)
endif()

mark_as_advanced(CLEAR Boost_INCLUDE_DIR)
find_package(Boost REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  mark_as_advanced(FORCE Boost_INCLUDE_DIR)
endif()

# Find HDF5_ROOT if not set
if ("$ENV{HDF5_ROOT}" STREQUAL "")
  find_file (HDF5_LIB libhdf5.so HINTS ENV LD_LIBRARY_PATH)
  if (NOT "${HDF5_LIB}" STREQUAL "")
    get_filename_component(HDF5_LIBRARY_DIR "${HDF5_LIB}" PATH)
    get_filename_component(HDF5_ROOT "${HDF5_LIBRARY_DIR}/../" ABSOLUTE)
    set(ENV{HDF5_ROOT} ${HDF5_ROOT})
  endif ()
endif ()

mark_as_advanced(CLEAR HDF5_C_INCLUDE_DIR)
mark_as_advanced(CLEAR HDF5_hdf5_LIBRARY_DEBUG)
mark_as_advanced(CLEAR HDF5_hdf5_LIBRARY_RELEASE)
set(HDF5_LIBRARIES ${vtkhdf5_LIBRARIES})
set(HDF5_hdf5_LIBRARY_RELEASE ${vtkhdf5_LIBRARIES})
set(HDF5_INCLUDE_DIRS ${vtkhdf5_INCLUDE_DIRS})
set(HDF5_C_INCLUDE_DIR ${vtkhdf5_INCLUDE_DIRS})
set(HDF5_FOUND TRUE)
find_package(HDF5 REQUIRED)
if(HDF5_FOUND)
  mark_as_advanced(FORCE HDF5_C_INCLUDE_DIR)
  mark_as_advanced(FORCE HDF5_hdf5_LIBRARY_DEBUG)
  mark_as_advanced(FORCE HDF5_hdf5_LIBRARY_RELEASE)
  include_directories(${HDF5_INCLUDE_DIRS})
  include_directories(${HDF5_C_INCLUDE_DIR})
  get_filename_component(HDF5_LIBRARY_DIR "${HDF5_hdf5_LIBRARY_RELEASE}" PATH)
  set(XDMF_LIBRARY_DIRS ${XDMF_LIBRARY_DIRS} ${HDF5_LIBRARY_DIR})
  set(HDF5_LIBRARIES ${HDF5_hdf5_LIBRARY_RELEASE})
  # FIXME: Would like to get this info from HDF5 so we don't have conflicting
  # MPI versions
  if(HDF5_IS_PARALLEL)
    if (NOT MPI_FOUND)
      find_package(MPI REQUIRED)
      if(MPI_FOUND)
        include_directories(${MPI_INCLUDE_PATH})
        set(XDMF_LIBRARIES ${XDMF_LIBRARIES} ${MPI_LIBRARY} ${MPI_EXTRA_LIBRARY})
        get_filename_component(MPI_LIBRARY_DIR ${MPI_LIBRARY} PATH)
        set(XDMF_LIBRARY_DIRS ${XDMF_LIBRARY_DIRS} ${MPI_LIBRARY_DIR})
      endif()
    endif ()
  endif()
  get_filename_component(HDF5_ROOT "${HDF5_INCLUDE_DIR}/../" REALPATH)
  set(HDF5_BINARY_DIRS ${HDF5_ROOT}/bin ${HDF5_ROOT}/dll)
  if (NOT HDF5_LIBRARIES)
    find_library (HDF5_LIBRARIES hdf5 HINTS ${HDF5_ROOT} ENV LD_LIBRARY_PATH)
  endif ()
  set(XDMF_LIBRARIES ${XDMF_LIBRARIES} ${HDF5_LIBRARIES})
  set(XDMF_HDF5_BINARY_DIRS ${HDF5_BINARY_DIRS} PARENT_SCOPE)
  set(XDMF_BINARIES ${XDMF_BINARIES} ${HDF5_BINARY_DIRS})
endif()


set(LIBXML2_LIBRARIES ${vtklibxml2_LIBRARIES})
set(LIBXML2_INCLUDE_DIR ${vtklibxml2_INCLUDE_DIRS})
set(LIBXML2_FOUND TRUE)
find_package(LibXml2 REQUIRED)
if(LIBXML2_FOUND)
  include_directories(${LIBXML2_INCLUDE_DIR})
  get_filename_component(LIBXML2_ROOT "${LIBXML2_INCLUDE_DIR}/../" REALPATH)
  set(LIBXML2_BINARY_DIRS ${LIBXML2_ROOT}/bin)
  set(LIBXML2_BINARY_DIRS ${LIBXML2_BINARY_DIRS} PARENT_SCOPE)
  set(XDMF_LIBRARIES ${XDMF_LIBRARIES} ${LIBXML2_LIBRARIES})
  set(XDMF_BINARIES ${XDMF_BINARIES} ${LIBXML2_BINARY_DIRS})
endif()

# Perform compile-time checks and generate XdmfCoreConfig.hpp

TEST_BIG_ENDIAN(XDMF_BIG_ENDIAN)

unset(HAVE_BOOST_SHARED_DYNAMIC_CAST CACHE)
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
  XdmfHeavyDataWriter
  XdmfInformation
  XdmfItem
  XdmfItemProperty
  XdmfSparseMatrix
  XdmfSubset
  XdmfSystemUtils
  ${CMAKE_CURRENT_BINARY_DIR}/XdmfVersion
  XdmfVisitor
  XdmfWriter)

add_library(XdmfCore ${LIBTYPE} ${XdmfCoreSources})
link_directories(${XDMF_LIBRARY_DIRS})

target_link_libraries(XdmfCore ${XDMF_LIBRARIES} ${vtkxdmf3_LINK_DEPENDS})
vtk_target_install(XdmfCore)

if(WIN32)
  add_definitions(-D_HDF5USEDLL_ -D_HDF5USEHLDLL_)
  if (BUILD_SHARED_LIBS)
    set_target_properties(XdmfCore PROPERTIES
        DEFINE_SYMBOL XdmfCore_EXPORTS)
  endif()
  if(NOT MSVC10)
	set_target_properties(XdmfCore PROPERTIES
      PREFIX ../
      IMPORT_PREFIX ../
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  endif()
endif()

if(XDMF_WRAP_JAVA)
  XDMF_SWIG_JAVA(XdmfCore)
endif()

if(XDMF_WRAP_PYTHON)
  XDMF_SWIG_PYTHON(XdmfCore)
endif()

set(XDMF_LIBRARY_DIRS ${XDMF_LIBRARY_DIRS} PARENT_SCOPE)
if(WIN32)
    set(XDMFCORE_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/XdmfCore.lib PARENT_SCOPE)
endif()

if(UNIX)
    set(XDMFCORE_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/libXdmfCore.so
        PARENT_SCOPE)
endif()

if(APPLE)
    set(XDMFCORE_LIBRARY ${CMAKE_INSTALL_PREFIX}/lib/libXdmfCore.dylib
        PARENT_SCOPE)
endif()

file(GLOB_RECURSE
  XdmfCoreHeaders
  "*.hpp" "*.tpp" "*.i"
  "../CMake/VersionSuite/*.hpp")
file(GLOB LokiHeaders loki/*.h)
install(FILES ${XdmfCoreHeaders} DESTINATION include)
install(FILES ${LokiHeaders} DESTINATION include/loki)
install(TARGETS XdmfCore
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

set(XdmfCore_INCLUDE_DIRS
  ${Boost_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${HDF5_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${PYTHON_INCLUDE_DIRS}
  CACHE INTERNAL "")

set(XDMF_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/include PARENT_SCOPE)
set(XDMF_LIBRARIES ${XDMF_LIBRARIES} ${CMAKE_INSTALL_PREFIX}/lib)
set(XDMF_DIR ${CMAKE_INSTALL_PREFIX} PARENT_SCOPE)
set(XDMF_BINARIES ${XDMF_BINARIES} ${CMAKE_INSTALL_PREFIX}/bin)

if(XDMF_BUILD_DSM)
  add_subdirectory(dsm)
endif()

#if(BUILD_TESTING)
#  add_subdirectory(tests)
#endif()

set(XDMF_LIBRARIES ${XDMF_LIBRARIES} PARENT_SCOPE)
set(XDMF_BINARIES ${XDMF_BINARIES} PARENT_SCOPE)

