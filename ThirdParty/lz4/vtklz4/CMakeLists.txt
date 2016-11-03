# XXX(kitware): File copied from cmake_unofficial/CMakeLists.txt and the modified
# to remove components not needed by ParaView e.g. options to build tools, CPack,
# etc.
cmake_minimum_required(VERSION 2.6)
PROJECT(LZ4)
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "LZ4 compression library")
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 5)
set(CPACK_PACKAGE_VERSION_PATCH r128)
set(VERSION_STRING	" \"${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}\" ")

include(CheckTypeSize)
include(GenerateExportHeader)

check_type_size("void *" SIZEOF_VOID_P)
IF( ${SIZEOF_VOID_P} STREQUAL  "8" )
    set (CMAKE_SYSTEM_PROCESSOR "64bit")
    MESSAGE( STATUS "64 bit architecture detected size of void * is " ${SIZEOF_VOID_P})
ENDIF()

# XXX(kitware): avoid quoted variable policy warning from CMake.
IF(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
   CMAKE_C_COMPILER_ID STREQUAL "Clang")
  SET(GNU_COMPATIBLE_COMPILER 1)
ENDIF()

if(GNU_COMPATIBLE_COMPILER)
  if(UNIX AND BUILD_LIBS)
    add_definitions(-fPIC)
  endif()
endif()

set(LZ4_DIR lib/)
set(LZ4_SRCS_LIB ${LZ4_DIR}lz4.c ${LZ4_DIR}lz4hc.c ${LZ4_DIR}lz4.h ${LZ4_DIR}lz4hc.h ${LZ4_DIR}lz4frame.c ${LZ4_DIR}lz4frame.h ${LZ4_DIR}xxhash.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${LZ4_DIR})
if(GNU_COMPATIBLE_COMPILER AND (NOT CMAKE_SYSTEM_NAME MATCHES "SunOS"))
   add_definitions("-std=c99")
endif()
add_definitions("-DLZ4_VERSION=\"${CPACK_PACKAGE_VERSION_PATCH}\"")

# XXX(kitware) adding exports for windows shared builds.
vtk_add_library(vtklz4 ${LZ4_SRCS_LIB} vtklz4Exports.h)
generate_export_header(vtklz4 EXPORT_FILE_NAME vtklz4Exports.h)
if(BUILD_SHARED_LIBS)
  # export flags are only added when building shared libs, they cause
  # mismatched visibility warnings when building statically since not all
  # libraries that VTK builds don't set visibility flags. Until we get a
  # time to do that, we skip visibility flags for static libraries.
  add_compiler_export_flags(my_abi_flags)
  set_property(TARGET vtklz4 APPEND PROPERTY COMPILE_FLAGS "${my_abi_flags}")
endif()

if(NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(FILES
    ${LZ4_DIR}/lz4.h
    ${LZ4_DIR}/lz4hc.h
    ${LZ4_DIR}/lz4frame.h
    ${LZ4_DIR}/vtklz4_mangle.h
    ${CMAKE_CURRENT_BINARY_DIR}/vtklz4Exports.h
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR}/vtklz4
    COMPONENT Developmetn)
endif()
