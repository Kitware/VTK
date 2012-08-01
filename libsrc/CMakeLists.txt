#-----------------------------------------------------------------------------
# Does Xdmf require support for 64 bit file systems
include(${CMAKE_CURRENT_SOURCE_DIR}/CheckCXXSourceRuns.cmake)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/vtkRequireLargeFilesSupport.cxx"
  XDMF_REQUIRE_LARGE_FILE_SUPPORT_FILE)
check_cxx_source_runs("${XDMF_REQUIRE_LARGE_FILE_SUPPORT_FILE}"
  CMAKE_REQUIRE_LARGE_FILE_SUPPORT "Xdmf support for 64 bit file systems")
set(XDMF_REQUIRE_LARGE_FILE_SUPPORT ${CMAKE_REQUIRE_LARGE_FILE_SUPPORT})

set( XdmfFormatSource
  XdmfArray
  XdmfDOM
  XdmfDataDesc
  XdmfExpression
  XdmfValues
  XdmfValuesXML
  XdmfValuesHDF
  XdmfValuesBinary
  XdmfHeavyData
  XdmfHDF
  XdmfHDFSupport
  XdmfLightData
  XdmfInformation
  XdmfDataItem
  XdmfDataStructure
  XdmfDataTransform
  XdmfElement
  XdmfObject
  )

set( XdmfExtraSource
  XdmfExprLex
  XdmfExprYacc
  )

set( XdmfModelSource
  XdmfRoot
  XdmfDomain
  XdmfAttribute
  XdmfGrid
  XdmfTopology
  XdmfTime
  XdmfRegion
  XdmfSet
  XdmfMap
  XdmfGeometry
  XdmfH5Driver
  XdmfDsm
  XdmfDsmBuffer
  XdmfDsmComm
  XdmfDsmMsg
  )

set(CSHARP_SOURCES
  SWIGTYPE_p_double.cs
  SWIGTYPE_p_float.cs
  SWIGTYPE_p_hid_t.cs
  SWIGTYPE_p_int.cs
  SWIGTYPE_p_istream.cs
  SWIGTYPE_p_long_long.cs
  SWIGTYPE_p_ostream.cs
  SWIGTYPE_p_p__xmlDoc.cs
  SWIGTYPE_p_void.cs
  SWIGTYPE_p__xmlNode.cs
  XdmfArray.cs
  XdmfArrayList.cs
  XdmfAttribute.cs
  XdmfCSharp.cs
  XdmfCSharpPINVOKE.cs
  XdmfDataDesc.cs
  XdmfDataItem.cs
  XdmfDataStructure.cs
  XdmfDomain.cs
  XdmfDOM.cs
  XdmfDsmComm.cs
  XdmfDsmCommMpi.cs
  XdmfDsm.cs
  XdmfElement.cs
  XdmfGeometry.cs
  XdmfGrid.cs
  XdmfHDF.cs
  XdmfHeavyData.cs
  XdmfInformation.cs
  XdmfLightData.cs
  XdmfMap.cs
  XdmfObject.cs
  XdmfRegion.cs
  XdmfRoot.cs
  XdmfSet.cs
  XdmfTime.cs
  XdmfTopology.cs
  XdmfValues.cs
  XdmfValuesHDF.cs
  XdmfValuesXML.cs
  XdmfDsmBuffer.cs
  XdmfDsmMsg.cs
  XdmfOpenCallback.cs
  XdmfCloseCallback.cs
  XdmfReadCallback.cs
  XdmfWriteCallback.cs
  )

set( XdmfGzipSource
  gzstream
  )

set( XdmfMpiSource
  XdmfDsmCommMpi
  )

set( XdmfMySQLSource
  XdmfValuesMySQL
  )


## system introspection - isn't this what Ice is for???
include (${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)
if(WIN32)
  check_type_size(__int64        SIZEOF___INT64)
endif()
check_type_size("long long"    SIZEOF_LONG_LONG)

include (${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
check_include_file(malloc.h XDMF_HAVE_MALLOC_H)

# System to Build
string(REGEX REPLACE "-" "_" XDMF_SYSTEM ${CMAKE_SYSTEM_NAME})

set(ARCH_TO_BUILD ${XDMF_SYSTEM} CACHE INTERNAL "Host Arcitecture : Linux IRIXN32 IRIX64 AIX CYGWIN")
# Allow the user to customize their build with some local options
#
add_definitions(-D${ARCH_TO_BUILD})

include (${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)

#TODO: why aren't we getting these from Ice?
if(VTK_SIZEOF_CHAR)
  set(XDMF_SIZEOF_CHAR   ${VTK_SIZEOF_CHAR})
  set(XDMF_SIZEOF_DOUBLE ${VTK_SIZEOF_DOUBLE})
  set(XDMF_SIZEOF_FLOAT  ${VTK_SIZEOF_FLOAT})
  set(XDMF_SIZEOF_INT    ${VTK_SIZEOF_INT})
  set(XDMF_SIZEOF_LONG   ${VTK_SIZEOF_LONG})
  set(XDMF_SIZEOF_SHORT  ${VTK_SIZEOF_SHORT})
else()
  check_type_size(char XDMF_SIZEOF_CHAR)
  check_type_size(double XDMF_SIZEOF_DOUBLE)
  check_type_size(float XDMF_SIZEOF_FLOAT)
  check_type_size(int XDMF_SIZEOF_INT)
  check_type_size(long XDMF_SIZEOF_LONG)
  check_type_size(short XDMF_SIZEOF_SHORT)
endif()

include (${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
if(WIN32)
  check_type_size(__int64        SIZEOF___INT64)
endif()
check_type_size("long long"    SIZEOF_LONG_LONG)
check_function_exists(strtoll HAVE_STRTOLL)

# Don't Really Need to check these ...
set(XDMF_VOID_VALID 1)
set(XDMF_VOID "void")
set(XDMF_PTR_VALID 1)
set(XDMF_PTR "void *")
set(XDMF_CHAR_VALID 1)
set(XDMF_CHAR "char")
set(XDMF_8_INT_VALID 1)
set(XDMF_8_INT  "char")
set(XDMF_8_U_INT  "unsigned char")
set(XDMF_16_INT_VALID 1)
set(XDMF_16_INT "short")
set(XDMF_16_U_INT "unsigned short")
# These should be Valid
if(${XDMF_SIZEOF_FLOAT} MATCHES 4)
  set(XDMF_32_FLOAT_VALID 1)
  set(XDMF_FLOAT "float")
  set(XDMF_32_FLOAT "float")
else ()
  message(SEND_ERROR "Can't Find a 32 Bit Float")
endif ()
if(${XDMF_SIZEOF_DOUBLE} MATCHES 8)
  set(XDMF_64_FLOAT_VALID 1)
  set(XDMF_DOUBLE "double")
  set(XDMF_64_FLOAT "double")
else ()
  message(SEND_ERROR "Can't Find a 64 Bit Float")
endif ()
# These are sometimes different
if(${XDMF_SIZEOF_INT} MATCHES 4)
  set(XDMF_32_INT_VALID 1)
  set(XDMF_32_INT "int")
  set(XDMF_32_U_INT "unsigned int")
  set(XDMF_32_S_INT "int")
else ()
  if(${XDMF_SIZEOF_LONG} MATCHES 4)
    set(XDMF_32_INT_VALID 1)
    set(XDMF_32_INT "long")
    set(XDMF_32_U_INT "unsigned long")
    set(XDMF_32_S_INT "long")
  else()
    message(SEND_ERROR "Can't Find a 32 Bit Integer")
  endif()
endif()

include(${xdmf2_SOURCE_DIR}/CMake/CheckFor64BitStreams.cmake)
check_for_64bit_streams(XDMF_HAVE_64BIT_STREAMS)

# Find include files
find_path(XDMF_HAVE_FCNTL fcntl.h /usr/include /usr/include/sys)
find_path(XDMF_HAVE_NETINET in.h /usr/include/netinet /usr/include /usr/include/sys)
find_path(XDMF_HAVE_MMAN mman.h /usr/include/sys /usr/include)
mark_as_advanced(XDMF_HAVE_FCNTL XDMF_HAVE_NETINET XDMF_HAVE_MMAN)

# Save out config to let other projects use xdmf
configure_file(${xdmf2_SOURCE_DIR}/libsrc/XdmfConfig.h.in
  ${xdmf2_BINARY_DIR}/libsrc/XdmfConfig.h)


if(NOT WIN32)
  add_definitions(-D_HPUX_SOURCE) #???
endif()

set(XdmfSources ${XdmfFormatSource} ${XdmfExtraSource} ${XdmfModelSource})


#optional portions of xdmf

#mySQL
option(XDMF_USE_MYSQL
  "Build Support for MySQL DataItems" OFF)
mark_as_advanced(XDMF_USE_MYSQL)
if(XDMF_USE_MYSQL)
    find_path(MYSQL_INCLUDE_PATH
        mysql.h
        /usr/include
        /usr/include/mysql
        /usr/local/include
        /usr/local/include/mysql)
    find_library(MYSQL_CLIENT_LIBRARY
        mysqlclient
        /usr/lib
        /usr/lib/mysql
        /usr/local/lib
        /usr/local/lib/mysql)
    set(XdmfSources ${XdmfSources} ${XdmfMySQLSource})
    add_definitions("-DXDMF_USE_MYSQL")
    include_directories(${MYSQL_INCLUDE_PATH})
endif()

# gzip compression
option(XDMF_USE_GZIP "Build GZip Compression" OFF)
if(XDMF_USE_GZIP)
  set(XdmfSources ${XdmfSources} ${XdmfGzipSource})
  add_definitions("-DXDMF_USE_GZIP")
endif()

# bz2lib compression
option(XDMF_USE_BZIP2 "Use bzip2" OFF)
if(XDMF_USE_BZIP2)
  find_package(BZip2)
  set(XDMF_BZIP2_LIBRARIES ${BZIP2_LIBRARIES})
  set(XDMF_BZIP2_INCLUDE_DIR ${BZIP2_INCLUDE_DIR})
  include_directories(${BZIP2_INCLUDE_DIR})
  add_definitions("-DXDMF_USE_BZIP2")
endif()

## MPI ###
if(XDMF_BUILD_MPI)
    set(XdmfSources ${XdmfSources} ${XdmfMpiSource})
else()
    add_definitions("-DXDMF_NO_MPI")
endif()

if(BUILD_SHARED_LIBS)
  set(LIBTYPE SHARED)
else()
  set(LIBTYPE STATIC)
endif()

#finally make the target that builds the xdmf library itself
add_library(xdmf2 ${LIBTYPE} ${XdmfSources})

#setup dependencies for the optional parts of library
if(XDMF_USE_MYSQL)
    add_definitions("-DXDMF_USE_MYSQL")
    include_directories(${MYSQL_INCLUDE_PATH})
    target_link_libraries(Xdmf ${MYSQL_CLIENT_LIBRARY} )
endif()

target_link_libraries(xdmf2
  ${XDMF_LIBXML2_LIBRARIES}
  ${XDMF_HDF5_LIBRARIES}
  ${XDMF_MPI_LIBRARIES}
  ${XDMF_ZLIB_LIBRARIES}
  ${XDMF_BZIP2_LIBRARIES})

if(NOT XDMF_INSTALL_NO_DEVELOPMENT)
  file(GLOB devFiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
  install(
      FILES ${devFiles}
      DESTINATION ${XDMF_INSTALL_INCLUDE_DIR}
      COMPONENT Development)
  install(
      FILES ${xdmf2_BINARY_DIR}/libsrc/XdmfConfig.h
      DESTINATION ${XDMF_INSTALL_INCLUDE_DIR}
      COMPONENT Development)
endif()

if(XDMF_INSTALL_EXPORT_NAME AND NOT XDMF_INSTALL_NO_LIBRARIES)
  install(TARGETS xdmf2
    EXPORT ${XDMF_INSTALL_EXPORT_NAME}
    RUNTIME DESTINATION ${XDMF_INSTALL_BIN_DIR} COMPONENT Runtime
    LIBRARY DESTINATION ${XDMF_INSTALL_LIB_DIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${XDMF_INSTALL_LIB_DIR} COMPONENT Development)
endif()
