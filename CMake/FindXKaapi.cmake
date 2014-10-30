# Try to find (x)Kaapi
# Once done, this will define
#
#  XKAAPI_FOUND - system has (x)Kaapi
#  XKAAPI_INCLUDE_DIRS - the (x)Kaapi include directories
#  XKAAPI_LIBRARIES - link these to use (x)Kaapi

set(XKAAPI_HOME $ENV{XKAAPI_HOME} CACHE PATH "Path to the (x)Kaapi install dir")

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_KAAPI QUIET kaapi++)
endif()

find_path(XKAAPI_INCLUDE_DIR kaapi++
  HINTS
  ${XKAAPI_HOME}/include
  ${PC_KAAPI_INCLUDEDIR}
  ${PC_KAAPI_INCLUDE_DIRS}
)

find_library(KAAPI_LIBRARY kaapi
  HINTS
  ${XKAAPI_HOME}/lib
  ${PC_KAAPI_LIBDIR}
  ${PC_KAAPI_LIBRARY_DIRS}
)
find_library(XKAAPI_LIBRARY kaapi++
  HINTS
  ${XKAAPI_HOME}/lib
  ${PC_KAAPI_LIBDIR}
  ${PC_KAAPI_LIBRARY_DIRS}
)

set(XKAAPI_LIBRARIES ${XKAAPI_LIBRARY} ${KAAPI_LIBRARY})
set(XKAAPI_INCLUDE_DIRS ${XKAAPI_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set XKAAPI_FOUND to TRUE if all listed variables are TRUE
find_package_handle_standard_args(XKaapi DEFAULT_MSG XKAAPI_LIBRARIES XKAAPI_INCLUDE_DIRS)

mark_as_advanced(XKAAPI_INCLUDE_DIR XKAAPI_LIBRARY KAAPI_LIBRARY KAAPI_C_LIBRARY KAAPI_FORTRAN_LIBRARY)
set(XKAAPI_LIBRARIES ${XKAAPI_LIBRARIES} ${CMAKE_THREAD_LIBS})
