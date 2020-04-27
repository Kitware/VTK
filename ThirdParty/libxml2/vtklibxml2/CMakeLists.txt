SET(PACKAGE "vtklibxml2")
SET(VERSION "2.6.27")
SET(LIBXML_VERSION_NUMBER 20627)
SET(LIBXML_VERSION_EXTRA "")
SET(MODULE_EXTENSION ${CMAKE_SHARED_MODULE_SUFFIX})

# Use new behavior of linking against libraries in the CMAKE_REQUIRED_LIBRARIES
# variable in the CHECK_INCLUDE_FILE macro
IF (POLICY CMP0075)
  CMAKE_POLICY(SET CMP0075 NEW)
ENDIF()

INCLUDE(CheckLibraryExists)
INCLUDE(CheckIncludeFile)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckSymbolExists)
INCLUDE(libxml2PlatformTests.cmake)

SET(LIBXML2_PLATFORM_TEST_FILE_C platformTestsC.c)

set(CMAKE_THREAD_PREFER_PTHREAD OFF)

FIND_PACKAGE(Threads)

# This macro checks if the symbol exists in the library and if it
# does, it appends library to the list.
SET(LIBXML2_LIBS "")
MACRO(CHECK_LIBRARY_EXISTS_CONCAT LIBRARY SYMBOL VARIABLE)
  CHECK_LIBRARY_EXISTS("${LIBRARY};${LIBXML2_LIBS}" ${SYMBOL} "" ${VARIABLE})
  IF(${VARIABLE})
    SET(LIBXML2_LIBS ${LIBXML2_LIBS} ${LIBRARY})
  ENDIF()
ENDMACRO()

CHECK_LIBRARY_EXISTS_CONCAT("dld"      shl_load       HAVE_SHLLOAD)

# Use needed system libraries.
SET(LIBXML2_LIBS ${LIBXML2_LIBS} ${CMAKE_THREAD_LIBS} ${CMAKE_DL_LIBS})
IF(UNIX)
  SET(LIBXML2_LIBS ${LIBXML2_LIBS} m)
ENDIF()

# Check for symbol dlopen separately from the dl library.
CHECK_LIBRARY_EXISTS("${LIBXML2_LIBS}" dlopen "" HAVE_DLOPEN)

# For other tests to use the same libraries
SET(CMAKE_REQUIRED_LIBRARIES ${LIBXML2_LIBS})
SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")

# Use VTK's zlib if we are building in the tree.
IF(TARGET VTK::zlib)
  SET(HAVE_LIBZ 1)
  SET(LIBXML2_LIBS VTK::zlib ${LIBXML2_LIBS})
ENDIF()

# Check if header file exists and add it to the list.
MACRO(CHECK_INCLUDE_FILE_CONCAT FILE VARIABLE)
  CHECK_INCLUDE_FILES("${LIBXML2_INCLUDES};${FILE}" ${VARIABLE})
  IF(${VARIABLE})
    SET(LIBXML2_INCLUDES ${LIBXML2_INCLUDES} ${FILE})
  ENDIF()
ENDMACRO()

CHECK_INCLUDE_FILE_CONCAT("sys/types.h"      HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE_CONCAT("dlfcn.h"          HAVE_DLFCN_H)
CHECK_INCLUDE_FILE_CONCAT("fcntl.h"          HAVE_FCNTL_H)
CHECK_INCLUDE_FILE_CONCAT("malloc.h"         HAVE_MALLOC_H)
CHECK_INCLUDE_FILE_CONCAT("inttypes.h"       HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE_CONCAT("limits.h"         HAVE_LIMITS_H)
CHECK_INCLUDE_FILE_CONCAT("stdint.h"         HAVE_STDINT_H)
CHECK_INCLUDE_FILE_CONCAT("stdio.h"          HAVE_STDIO_H)
CHECK_INCLUDE_FILE_CONCAT("stdlib.h"         HAVE_STDLIB_H)
CHECK_INCLUDE_FILE_CONCAT("string.h"         HAVE_STRING_H)
CHECK_INCLUDE_FILE_CONCAT("sys/stat.h"       HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_CONCAT("time.h"           HAVE_TIME_H)
CHECK_INCLUDE_FILE_CONCAT("unistd.h"         HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_CONCAT("signal.h"         HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE_CONCAT("errno.h"          HAVE_ERRNO_H)
CHECK_INCLUDE_FILE_CONCAT("ansidecl.h"       HAVE_ANSIDECL_H)
CHECK_INCLUDE_FILE_CONCAT("ctype.h"          HAVE_CTYPE_H)
CHECK_INCLUDE_FILE_CONCAT("dl.h"             HAVE_DL_H)
CHECK_INCLUDE_FILE_CONCAT("float.h"          HAVE_FLOAT_H)
CHECK_INCLUDE_FILE_CONCAT("math.h"           HAVE_MATH_H)
CHECK_INCLUDE_FILE_CONCAT("pthread.h"        HAVE_PTHREAD_H)

CHECK_SYMBOL_EXISTS(localtime     "${LIBXML2_INCLUDES}" HAVE_LOCALTIME)
CHECK_SYMBOL_EXISTS(strftime      "${LIBXML2_INCLUDES}" HAVE_STRFTIME)
CHECK_SYMBOL_EXISTS(snprintf      "${LIBXML2_INCLUDES}" HAVE_SNPRINTF)
CHECK_SYMBOL_EXISTS(stat          "${LIBXML2_INCLUDES}" HAVE_STAT)
CHECK_SYMBOL_EXISTS(_stat         "${LIBXML2_INCLUDES}" HAVE__STAT)
CHECK_SYMBOL_EXISTS(vsnprintf     "${LIBXML2_INCLUDES}" HAVE_VSNPRINTF)

LIBXML2_PLATFORM_C_TEST(HAVE_VA_COPY     "Checking for va_copy" DIRECT)
LIBXML2_PLATFORM_C_TEST(HAVE___VA_COPY   "Checking for __va_copy" DIRECT)

set(HAVE_ZLIB_H 1)

SET(WITH_TRIO 0) # some kind of 3rd party library
SET(WITH_THREADS 1)
SET(WITH_THREAD_ALLOC 1)
SET(WITH_TREE 1)
SET(WITH_OUTPUT 1)
SET(WITH_PUSH 1)
SET(WITH_READER 1)
SET(WITH_PATTERN 1)
SET(WITH_WRITER 1)
SET(WITH_SAX1 1)
SET(WITH_FTP 0)
SET(WITH_HTTP 0)
SET(WITH_VALID 1)
SET(WITH_HTML 1)
SET(WITH_LEGACY 0)
SET(WITH_C14N 1)
SET(WITH_CATALOG 1)
SET(WITH_DOCB 1)
SET(WITH_XPATH 1)
SET(WITH_XPTR 1)
SET(WITH_XINCLUDE 1)
SET(WITH_ICONV 0)
SET(WITH_ICU 0)
SET(WITH_ISO8859X 1)
SET(WITH_DEBUG 0)
SET(WITH_MEM_DEBUG 0)
SET(WITH_RUN_DEBUG 0)
SET(WITH_REGEXPS 1)
SET(WITH_SCHEMAS 1)
SET(WITH_SCHEMATRON 1)
SET(WITH_MODULES 1)
SET(WITH_ZLIB 1)
SET(WITH_LZMA 0)

# not building VTK with pthreads
IF(NOT VTK_USE_PTHREADS AND HAVE_PTHREAD_H)
  SET(HAVE_PTHREAD_H 0)
ENDIF()

set(sources
  DOCBparser.c
  HTMLparser.c
  HTMLtree.c
  SAX.c
  SAX2.c
  buf.c
  c14n.c
  catalog.c
  chvalid.c
  debugXML.c
  dict.c
  encoding.c
  entities.c
  error.c
  globals.c
  hash.c
  legacy.c
  list.c
  parser.c
  parserInternals.c
  pattern.c
  relaxng.c
  schematron.c
  threads.c
  tree.c
  uri.c
  valid.c
  xinclude.c
  xlink.c
  xmlIO.c
  xmlmemory.c
  xmlmodule.c
  xmlreader.c
  xmlregexp.c
  xmlsave.c
  xmlschemas.c
  xmlschemastypes.c
  xmlstring.c
  xmlunicode.c
  xmlwriter.c
  xpath.c
  xpointer.c)

# Create the platform configuration header.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/config_cmake.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/libxml/config.h"
  @ONLY)

# Create the build configuration header.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/include/libxml/xmlversion.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/libxml/xmlversion.h"
  @ONLY)

set(headers
  include/libxml/c14n.h
  include/libxml/catalog.h
  include/libxml/chvalid.h
  include/libxml/debugXML.h
  include/libxml/dict.h
  include/libxml/DOCBparser.h
  include/libxml/encoding.h
  include/libxml/entities.h
  include/libxml/globals.h
  include/libxml/hash.h
  include/libxml/HTMLparser.h
  include/libxml/HTMLtree.h
  include/libxml/list.h
  include/libxml/nanoftp.h
  include/libxml/nanohttp.h
  include/libxml/parser.h
  include/libxml/parserInternals.h
  include/libxml/pattern.h
  include/libxml/relaxng.h
  include/libxml/SAX.h
  include/libxml/SAX2.h
  include/libxml/schemasInternals.h
  include/libxml/schematron.h
  include/libxml/threads.h
  include/libxml/tree.h
  include/libxml/uri.h
  include/libxml/valid.h
  include/libxml/vtk_libxml2_mangle.h
  include/libxml/xinclude.h
  include/libxml/xlink.h
  include/libxml/xmlautomata.h
  include/libxml/xmlerror.h
  include/libxml/xmlexports.h
  include/libxml/xmlIO.h
  include/libxml/xmlmemory.h
  include/libxml/xmlmodule.h
  include/libxml/xmlreader.h
  include/libxml/xmlregexp.h
  include/libxml/xmlsave.h
  include/libxml/xmlschemas.h
  include/libxml/xmlschemastypes.h
  include/libxml/xmlstring.h
  include/libxml/xmlunicode.h
  include/libxml/xmlwin32version.h
  include/libxml/xmlwriter.h
  include/libxml/xpath.h
  include/libxml/xpathInternals.h
  include/libxml/xpointer.h
  "${CMAKE_CURRENT_BINARY_DIR}/include/libxml/xmlversion.h")

if (VTK_USE_WIN32_THREADS)
  # threads.c looks for this
  set_source_files_properties(threads.c
    PROPERTIES
      COMPILE_DEFINITIONS HAVE_WIN32_THREADS)
endif ()

vtk_module_add_module(VTK::libxml2
  SOURCES ${sources}
  HEADERS ${headers}
  HEADERS_SUBDIR "vtklibxml2/include/libxml")
target_compile_definitions(libxml2
  PRIVATE
    _GNU_SOURCE
    _REENTRANT)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(libxml2
    PRIVATE
      LIBXML_STATIC)
endif()

target_include_directories(libxml2
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include/libxml"
    "${CMAKE_CURRENT_BINARY_DIR}/include/libxml"
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>"
    "$<INSTALL_INTERFACE:${_vtk_build_HEADERS_DESTINATION}/vtklibxml2/include>")
