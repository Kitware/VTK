PROJECT(vtklibxml2 C)
INCLUDE_REGULAR_EXPRESSION("^.*$")

SET(PACKAGE "vtklibxml2")
SET(VERSION "2.6.27")
SET(LIBXML_VERSION_NUMBER 20627)
SET(LIBXML_VERSION_EXTRA "")
SET(MODULE_EXTENSION ${CMAKE_SHARED_MODULE_SUFFIX})

SET(PACKAGE_TARNAME "")
SET(PACKAGE_BUGREPORT " ")
SET(PACKAGE_NAME "")
SET(PACKAGE_VERSION "")
SET(PACKAGE_STRING "")
SET(PACKAGE_BUGREPORT "")

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
  ENDIF(${VARIABLE})
ENDMACRO(CHECK_LIBRARY_EXISTS_CONCAT)

CHECK_LIBRARY_EXISTS_CONCAT("dl"       dlopen         HAVE_LIBDL)
CHECK_LIBRARY_EXISTS_CONCAT("dld"      shl_load       HAVE_SHLLOAD)
# VTK does not use the read line functionality of libxml2, so don't
# try and find these libraries.  It creates extra dependencies on 
# executables and libraries.  This might cause trouble when creating
# packages or rpm's.

#CHECK_LIBRARY_EXISTS_CONCAT("history"  append_history HAVE_LIBHISTORY)
#CHECK_LIBRARY_EXISTS_CONCAT("readline" readline       HAVE_LIBREADLINE)
IF(NOT vtkzlib_LIBRARIES)
  CHECK_LIBRARY_EXISTS_CONCAT("z"      inflateEnd     HAVE_LIBZ)
ENDIF(NOT vtkzlib_LIBRARIES)

# Use needed system libraries.
SET(LIBXML2_LIBS ${LIBXML2_LIBS} ${CMAKE_THREAD_LIBS} ${CMAKE_DL_LIBS})
IF(UNIX)
  SET(LIBXML2_LIBS ${LIBXML2_LIBS} m)
ENDIF(UNIX)

# Check for symbol dlopen separately from the dl library.
CHECK_LIBRARY_EXISTS("${LIBXML2_LIBS}" dlopen "" HAVE_DLOPEN)

# For other tests to use the same libraries
SET(CMAKE_REQUIRED_LIBRARIES ${LIBXML2_LIBS})
SET(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")

# Use VTK's zlib if we are building in the tree.
IF(vtkzlib_LIBRARIES)
  SET(HAVE_LIBZ 1)
  SET(LIBXML2_LIBS ${vtkzlib_LIBRARIES} ${LIBXML2_LIBS})
ENDIF(vtkzlib_LIBRARIES)

# Check if header file exists and add it to the list.
MACRO(CHECK_INCLUDE_FILE_CONCAT FILE VARIABLE)
  CHECK_INCLUDE_FILES("${LIBXML2_INCLUDES};${FILE}" ${VARIABLE})
  IF(${VARIABLE})
    SET(LIBXML2_INCLUDES ${LIBXML2_INCLUDES} ${FILE})
  ENDIF(${VARIABLE})
ENDMACRO(CHECK_INCLUDE_FILE_CONCAT)

# Check for headers not actually needed for the configured header but
# that may be needed to include the other headers tested.
CHECK_INCLUDE_FILE_CONCAT("stdio.h"          HAVE_STDIO_H)
CHECK_INCLUDE_FILE_CONCAT("stddef.h"         HAVE_STDDEF_H)

CHECK_INCLUDE_FILE_CONCAT("sys/types.h"      HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE_CONCAT("inttypes.h"       HAVE_INTTYPES_H)
CHECK_INCLUDE_FILE_CONCAT("dlfcn.h"          HAVE_DLFCN_H)
CHECK_INCLUDE_FILE_CONCAT("fcntl.h"          HAVE_FCNTL_H)
CHECK_INCLUDE_FILE_CONCAT("malloc.h"         HAVE_MALLOC_H)
CHECK_INCLUDE_FILE_CONCAT("memory.h"         HAVE_MEMORY_H)
CHECK_INCLUDE_FILE_CONCAT("netdb.h"          HAVE_NETDB_H)
CHECK_INCLUDE_FILE_CONCAT("limits.h"         HAVE_LIMITS_H)
CHECK_INCLUDE_FILE_CONCAT("sys/socket.h"     HAVE_SYS_SOCKET_H)
CHECK_INCLUDE_FILE_CONCAT("netinet/in.h"     HAVE_NETINET_IN_H)
CHECK_INCLUDE_FILE_CONCAT("sys/select.h"     HAVE_SYS_SELECT_H)
CHECK_INCLUDE_FILE_CONCAT("stdint.h"         HAVE_STDINT_H)
CHECK_INCLUDE_FILE_CONCAT("stdlib.h"         HAVE_STDLIB_H)
CHECK_INCLUDE_FILE_CONCAT("string.h"         HAVE_STRING_H)
CHECK_INCLUDE_FILE_CONCAT("strings.h"        HAVE_STRINGS_H)
CHECK_INCLUDE_FILE_CONCAT("sys/stat.h"       HAVE_SYS_STAT_H)
CHECK_INCLUDE_FILE_CONCAT("sys/time.h"       HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE_CONCAT("time.h"           HAVE_TIME_H)
CHECK_INCLUDE_FILE_CONCAT("unistd.h"         HAVE_UNISTD_H)
CHECK_INCLUDE_FILE_CONCAT("signal.h"         HAVE_SIGNAL_H)
CHECK_INCLUDE_FILE_CONCAT("errno.h"          HAVE_ERRNO_H)
CHECK_INCLUDE_FILE_CONCAT("ansidecl.h"       HAVE_ANSIDECL_H)
CHECK_INCLUDE_FILE_CONCAT("arpa/inet.h"      HAVE_ARPA_INET_H)
CHECK_INCLUDE_FILE_CONCAT("arpa/nameser.h"   HAVE_ARPA_NAMESER_H)
CHECK_INCLUDE_FILE_CONCAT("ctype.h"          HAVE_CTYPE_H)
CHECK_INCLUDE_FILE_CONCAT("dirent.h"         HAVE_DIRENT_H)
CHECK_INCLUDE_FILE_CONCAT("dl.h"             HAVE_DL_H)
CHECK_INCLUDE_FILE_CONCAT("float.h"          HAVE_FLOAT_H)
CHECK_INCLUDE_FILE_CONCAT("fp_class.h"       HAVE_FP_CLASS_H)
CHECK_INCLUDE_FILE_CONCAT("ieeefp.h"         HAVE_IEEEFP_H)
CHECK_INCLUDE_FILE_CONCAT("math.h"           HAVE_MATH_H)
CHECK_INCLUDE_FILE_CONCAT("nan.h"            HAVE_NAN_H)
CHECK_INCLUDE_FILE_CONCAT("ndir.h"           HAVE_NDIR_H)
CHECK_INCLUDE_FILE_CONCAT("pthread.h"        HAVE_PTHREAD_H)
CHECK_INCLUDE_FILE_CONCAT("resolv.h"         HAVE_RESOLV_H)
CHECK_INCLUDE_FILE_CONCAT("stdarg.h"         HAVE_STDARG_H)
CHECK_INCLUDE_FILE_CONCAT("sys/mman.h"       HAVE_SYS_MMAN_H)
CHECK_INCLUDE_FILE_CONCAT("sys/timeb.h"      HAVE_SYS_TIMEB_H)

CHECK_SYMBOL_EXISTS(finite        "${LIBXML2_INCLUDES}" HAVE_FINITE)
CHECK_SYMBOL_EXISTS(fpclass       "${LIBXML2_INCLUDES}" HAVE_FPCLASS)
CHECK_SYMBOL_EXISTS(fp_class      "${LIBXML2_INCLUDES}" HAVE_FP_CLASS)
CHECK_SYMBOL_EXISTS(fprintf       "${LIBXML2_INCLUDES}" HAVE_FPRINTF)
CHECK_SYMBOL_EXISTS(ftime         "${LIBXML2_INCLUDES}" HAVE_FTIME)
CHECK_SYMBOL_EXISTS(gettimeofday  "${LIBXML2_INCLUDES}" HAVE_GETTIMEOFDAY)
CHECK_SYMBOL_EXISTS(localtime     "${LIBXML2_INCLUDES}" HAVE_LOCALTIME)
CHECK_SYMBOL_EXISTS(printf        "${LIBXML2_INCLUDES}" HAVE_PRINTF)
CHECK_SYMBOL_EXISTS(sscanf        "${LIBXML2_INCLUDES}" HAVE_SSCANF)
CHECK_SYMBOL_EXISTS(sprintf       "${LIBXML2_INCLUDES}" HAVE_SPRINTF)
CHECK_SYMBOL_EXISTS(snprintf      "${LIBXML2_INCLUDES}" HAVE_SNPRINTF)
CHECK_SYMBOL_EXISTS(signal        "${LIBXML2_INCLUDES}" HAVE_SIGNAL)
CHECK_SYMBOL_EXISTS(strdup        "${LIBXML2_INCLUDES}" HAVE_STRDUP)
CHECK_SYMBOL_EXISTS(strndup       "${LIBXML2_INCLUDES}" HAVE_STRNDUP)
CHECK_SYMBOL_EXISTS(strerror      "${LIBXML2_INCLUDES}" HAVE_STRERROR)
CHECK_SYMBOL_EXISTS(strftime      "${LIBXML2_INCLUDES}" HAVE_STRFTIME)
CHECK_SYMBOL_EXISTS(stat          "${LIBXML2_INCLUDES}" HAVE_STAT)
CHECK_SYMBOL_EXISTS(_stat         "${LIBXML2_INCLUDES}" HAVE__STAT)
CHECK_SYMBOL_EXISTS(vfprintf      "${LIBXML2_INCLUDES}" HAVE_VFPRINTF)
CHECK_SYMBOL_EXISTS(vsnprintf     "${LIBXML2_INCLUDES}" HAVE_VSNPRINTF)
CHECK_SYMBOL_EXISTS(vsprintf      "${LIBXML2_INCLUDES}" HAVE_VSPRINTF)

LIBXML2_PLATFORM_C_TEST(HAVE_GETADDRINFO "Checking for getaddrinfo." DIRECT)
LIBXML2_PLATFORM_C_TEST(HAVE_SOCKLEN_T   "Checking for socklen_t" DIRECT)
LIBXML2_PLATFORM_C_TEST(HAVE_SYS_DIR_H   "Checking for DIR in sys/dir.h" DIRECT)
LIBXML2_PLATFORM_C_TEST(HAVE_SYS_NDIR_H  "Checking for DIR in sys/ndir.h" DIRECT)
LIBXML2_PLATFORM_C_TEST(HAVE_VA_COPY     "Checking for va_copy" DIRECT)
LIBXML2_PLATFORM_C_TEST(HAVE___VA_COPY   "Checking for __va_copy" DIRECT)
LIBXML2_PLATFORM_C_TEST(SUPPORT_IP6      "Checking for ipv6 support." DIRECT)

# Use VTK's zlib if we are building in the tree.
IF(vtkzlib_LIBRARIES)
  SET(HAVE_VTK_ZLIB_H 1)
  SET(HAVE_ZLIB_H 1)
ELSE(vtkzlib_LIBRARIES)
  CHECK_INCLUDE_FILE_CONCAT("zlib.h"           HAVE_ZLIB_H)
ENDIF(vtkzlib_LIBRARIES)

IF(HAVE_SOCKLEN_T)
  SET(XML_SOCKLEN_T "socklen_t")
ELSE(HAVE_SOCKLEN_T)
  SET(XML_SOCKLEN_T)
ENDIF(HAVE_SOCKLEN_T)
SET(_WINSOCKAPI_)


SET(WITH_TRIO 0) # some kind of 3rd party library
SET(WITH_THREADS 1)
SET(WITH_TREE 1)
SET(WITH_OUTPUT 1)
SET(WITH_PUSH 1)
SET(WITH_READER 1)
SET(WITH_PATTERN 1)
SET(WITH_WRITER 1)
SET(WITH_SAX1 1)
SET(WITH_FTP 0)  # fix socket API
SET(WITH_HTTP 0) # fix socket API
SET(WITH_VALID 1)
SET(WITH_HTML 1)
SET(WITH_LEGACY 1)
SET(WITH_C14N 1)
SET(WITH_CATALOG 1)
SET(WITH_DOCB 1)
SET(WITH_XPATH 1)
SET(WITH_XPTR 1)
SET(WITH_XINCLUDE 1)
SET(WITH_ICONV 0)
SET(WITH_ISO8859X 1)
SET(WITH_DEBUG 0)
SET(WITH_MEM_DEBUG 0)
SET(WITH_RUN_DEBUG 0)
SET(WITH_REGEXPS 1)
SET(WITH_REGEXPS 1)
SET(WITH_REGEXPS 1)
SET(WITH_SCHEMAS 1)
SET(WITH_SCHEMAS 1)
SET(WITH_SCHEMATRON 1)
SET(WITH_MODULES 1)

IF(HAVE_ZLIB_H AND HAVE_LIBZ)
  SET(WITH_ZLIB 1)
ELSE(HAVE_ZLIB_H AND HAVE_LIBZ)
  SET(WITH_ZLIB 0)
ENDIF(HAVE_ZLIB_H AND HAVE_LIBZ)

# not building VTK with pthreads
IF(NOT VTK_USE_PTHREADS AND HAVE_PTHREAD_H)
  SET(HAVE_PTHREAD_H 0)
ENDIF()

# Create the platform configuration header.
CONFIGURE_FILE(${vtklibxml2_SOURCE_DIR}/config_cmake.h.in
               ${vtklibxml2_BINARY_DIR}/config.h @ONLY)

# Create the build configuration header.
CONFIGURE_FILE(${vtklibxml2_SOURCE_DIR}/include/libxml/xmlversion.h.in
               ${vtklibxml2_BINARY_DIR}/libxml/xmlversion.h @ONLY)
SET(vtklibxml2_HEADER_FILES
  ${vtklibxml2_BINARY_DIR}/libxml/xmlversion.h)

# Copy all the source headers over to the same directory as the build
# configuration header so that headers can include each other with
# double quotes.
FILE(GLOB vtklibxml2_HEADERS
  RELATIVE ${vtklibxml2_SOURCE_DIR}/include/libxml
  include/libxml/*.h)
FOREACH(f ${vtklibxml2_HEADERS})
  CONFIGURE_FILE(${vtklibxml2_SOURCE_DIR}/include/libxml/${f}
                 ${vtklibxml2_BINARY_DIR}/libxml/${f} COPYONLY)
  SET(vtklibxml2_HEADER_FILES ${vtklibxml2_HEADER_FILES}
    ${vtklibxml2_BINARY_DIR}/libxml/${f})
ENDFOREACH(f)

# Block warnings unless we are instructed to allow them.
# SET(VTKLIBXML2_WARNINGS_ALLOW 1)
VTK_THIRD_PARTY_WARNING_SUPPRESS(VTKLIBXML2 C)

ADD_DEFINITIONS(-DHAVE_CONFIG_H -D_GNU_SOURCE -DPIC -D_REENTRANT)
IF(WIN32 AND NOT CYGWIN)
  # threads.c looks for this
  ADD_DEFINITIONS(-DHAVE_WIN32_THREADS)
ENDIF(WIN32 AND NOT CYGWIN)

SET(vtklibxml2_la_SOURCES
  DOCBparser.c
  HTMLparser.c
  HTMLtree.c
  SAX.c
  SAX2.c
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
  nanoftp.c
  nanohttp.c
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
  xpointer.c
  )

INCLUDE_DIRECTORIES(BEFORE ${vtklibxml2_BINARY_DIR})

IF(NOT BUILD_SHARED_LIBS)
  ADD_DEFINITIONS(-DLIBXML_STATIC)
ENDIF(NOT BUILD_SHARED_LIBS)
VTK_ADD_LIBRARY(vtklibxml2 ${vtklibxml2_la_SOURCES})
TARGET_LINK_LIBRARIES(vtklibxml2 ${LIBXML2_LIBS})

IF(NOT VTK_INSTALL_NO_DEVELOPMENT)
  INSTALL(FILES
    ${vtklibxml2_HEADER_FILES}
    DESTINATION ${VTK_INSTALL_INCLUDE_DIR_CM24}/vtklibxml2/libxml
    COMPONENT Development)
ENDIF(NOT VTK_INSTALL_NO_DEVELOPMENT)
