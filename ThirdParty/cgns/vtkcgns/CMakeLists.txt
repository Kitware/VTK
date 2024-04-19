if (FALSE) # XXX(kitware): VTK handles versions.
cmake_minimum_required(VERSION 3.8)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.10")
  cmake_policy(SET CMP0015 NEW)
endif()
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
  cmake_policy(SET CMP0081 NEW)
endif()

set(CMAKE_BUILD_TYPE "Release" CACHE STRING
  "one of: Release, Debug, RelWithDebInfo or MinSizeRel")
endif ()

project("cgns" C)

# Determine CGNS_VERSION from src/cgnslib.h for 
file (READ ${PROJECT_SOURCE_DIR}/src/cgnslib.h _cgnslib_h_contents)
string (REGEX REPLACE ".*#define[ \t]+CGNS_DOTVERS[ \t]+([0-9]*)\\.([0-9])[0-9]*.*$"
    "\\1.\\2" CGNS_VERSION ${_cgnslib_h_contents})

if (FALSE) # XXX(kitware): VTK doesn't need CPack here.
# Allow for building a package
set(CPACK_PACKAGE_VERSION "${CGNS_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "cgns-${CGNS_VERSION}")
set(CPACK_GENERATOR "STGZ;TGZ;ZIP")
set(CPACK_SOURCE_GENERATOR "STGZ;TGZ;ZIP")
include(CPack)

CONFIGURE_FILE(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
	"${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
endif ()

if (FALSE) # XXX(kitware): CMake no longer does this.
# if this is cygwin, turn off win32 flag
if (CYGWIN)
  set(WIN32 "NO")
endif (CYGWIN)
endif ()

##############
# build mode #
##############

# get some type sizes
include(CheckTypeSize)
CHECK_TYPE_SIZE(long SIZEOF_LONG)
CHECK_TYPE_SIZE(off_t SIZEOF_OFF_T)

if (FALSE) # XXX(kitware): Hardcoded settings.
if ("${SIZEOF_OFF_T}" LESS 8)
  option(CGNS_ENABLE_LFS "Enable or disable large file support" "OFF")
endif ("${SIZEOF_OFF_T}" LESS 8)

if ("${CMAKE_SIZEOF_VOID_P}" GREATER 4)
  option(CGNS_ENABLE_64BIT "Enable or disable 64-bit code building" "ON")
else ("${CMAKE_SIZEOF_VOID_P}" GREATER 4)
  set(CGNS_ENABLE_64BIT "OFF")
endif ("${CMAKE_SIZEOF_VOID_P}" GREATER 4)

option(CGNS_ENABLE_LEGACY "Enable or disable building legacy code (3.0 compatible)" "OFF")
option(CGNS_ENABLE_SCOPING "Enable or disable scoping of enumeration values" "OFF")
option(CGNS_ENABLE_BASE_SCOPE "Enable or disable base scoped families or connectivities" "OFF")
option(CGNS_ENABLE_MEM_DEBUG "Enable or disable memory debugging" "OFF")

set(CGNS_BUILD_SHARED "ON" CACHE BOOL "Build a shared version of the library")
else ()
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CGNS_ENABLE_64BIT ON)
else ()
  set(CGNS_ENABLE_64BIT OFF)
endif ()
if ("${SIZEOF_OFF_T}" LESS 8)
  set(CGNS_ENABLE_LFS ON) # needed on 32bit systems
else ()
  set(CGNS_ENABLE_LFS OFF)
endif ()

set(CGNS_ENABLE_LEGACY OFF)
set(CGNS_ENABLE_SCOPING ON)
set(CGNS_ENABLE_BASE_SCOPE OFF)
set(CGNS_ENABLE_MEM_DEBUG OFF)
endif ()

if (CGNS_ENABLE_LEGACY)
  set(CGNS_ENABLE_64BIT "OFF")
  mark_as_advanced(FORCE CGNS_ENABLE_64BIT)
else (CGNS_ENABLE_LEGACY)
  mark_as_advanced(CLEAR CGNS_ENABLE_64BIT)
endif (CGNS_ENABLE_LEGACY)

if (CGNS_ENABLE_64BIT)
  set(CGNS_ENABLE_LEGACY "OFF")
  mark_as_advanced(FORCE CGNS_ENABLE_LEGACY)
else (CGNS_ENABLE_64BIT)
  mark_as_advanced(CLEAR CGNS_ENABLE_LEGACY)
endif (CGNS_ENABLE_64BIT)
	
if (CGNS_ENABLE_LFS)
  include(CheckSymbolExists)
  if (WIN32)
    # Windows does not have an _open64 function
    set(HAVE_OPEN64 0)
    check_symbol_exists(_lseeki64 "io.h" HAVE_LSEEK64)
  else (WIN32)
    check_symbol_exists(open64 "sys/types.h;sys/stat.h;unistd.h" HAVE_OPEN64)
    check_symbol_exists(lseek64 "sys/types.h;unistd.h" HAVE_LSEEK64)
  endif (WIN32)
  if (HAVE_OPEN64)
    add_definitions(-DHAVE_OPEN64)
  endif (HAVE_OPEN64)
  if (HAVE_LSEEK64)
    add_definitions(-DHAVE_LSEEK64)
  endif (HAVE_LSEEK64)
else (CGNS_ENABLE_LFS)
  remove_definitions(-DHAVE_OPEN64 -DHAVE_LSEEK64)
endif (CGNS_ENABLE_LFS)

# XXX(kitware): Hardcoded for ParaView.
if (FALSE)
option(CGNS_ENABLE_TESTS "Enable test programs" "OFF")
else ()
set(CGNS_ENABLE_TESTS OFF)
endif ()
if (CGNS_ENABLE_TESTS)
  enable_testing()
endif (CGNS_ENABLE_TESTS)

###########
# Fortran #
###########

if (FALSE) # XXX(kitware): Hardcoded settings.
# Allow fortran support to be toggled
option(CGNS_ENABLE_FORTRAN "Enable or disable the use of Fortran" "OFF")
if (NOT DEFINED FORTRAN_NAMING)
  set(FORTRAN_NAMING "UNKNOWN")
endif (NOT DEFINED FORTRAN_NAMING)
else ()
set(CGNS_ENABLE_FORTRAN OFF)
endif ()

# If we are using fortran, enable it and configure the C-Fortran interface
# It would be nice to automatically detect this setting
if(CGNS_ENABLE_FORTRAN)
  enable_language("Fortran" OPTIONAL)

  #-----------------------------------------------------------------------------
  # Detect name mangling convention used between Fortran and C
  #-----------------------------------------------------------------------------
  set(F2CLIST  "LOWERCASE" "LOWERCASE_" "LOWERCASE__" "UPPERCASE" "UPPERCASE_" "UPPERCASE__")
  set(FORTRAN_NAMING_HELP
  "Configures how to link the Fortran components into the C library.
  Options are:
  	  LOWERCASE
	  LOWERCASE_
	  LOWERCASE__
	  UPPERCASE
	  UPPERRCASE_
	  UPPERRCASE__
  LOWERCASE_ is known to work with gfortran.  If this setting
  does not work with your compiler, try the others until one does.
  ")

  include(FortranCInterface)
  set(FortranCInterface_GLOBAL_SYMBOLS mysub mysub_ mysub__ MYSUB MYSUB_ MYSUB__)
  FortranCInterface_HEADER(${CMAKE_BINARY_DIR}/FCMangle.h 
  			   MACRO_NAMESPACE "CGNS_FC_"
  			   SYMBOL_NAMESPACE "CGNS_FC_")

  file (STRINGS ${CMAKE_BINARY_DIR}/FCMangle.h CONTENTS REGEX "CGNS_FC_GLOBAL\\(.*,.*\\) +(.*)")
  string (REGEX MATCH "CGNS_FC_GLOBAL\\(.*,.*\\) +(.*)" RESULT ${CONTENTS})
  set (CGNS_FC_FUNC "${CMAKE_MATCH_1}")

  file(REMOVE ${CMAKE_BINARY_DIR}/FCMangle.h)

  if(CGNS_FC_FUNC MATCHES "^NAME(.*)")
    set (FORTRAN_NAMING "UPPERCASE")
  elseif(CGNS_FC_FUNC MATCHES "^name(.*)")
    set (FORTRAN_NAMING "LOWERCASE")
  endif()
  if(CGNS_FC_FUNC MATCHES "(.*)__$")
    set (FORTRAN_NAMING "${FORTRAN_NAMING}__")
  elseif(CGNS_FC_FUNC MATCHES "(.*)_$")
    set (FORTRAN_NAMING "${FORTRAN_NAMING}_")
  endif()

  message(STATUS "Fortran name mangling convention: ${FORTRAN_NAMING}")

  # check that FORTRAN_NAMING is valid
  if (FORTRAN_NAMING STREQUAL "UNKNOWN")
    if (WIN32)
      set(FORTRAN_NAMING "UPPERCASE" CACHE STRING ${FORTRAN_NAMING_HELP})
    else (WIN32)
      set(FORTRAN_NAMING "LOWERCASE_" CACHE STRING ${FORTRAN_NAMING_HELP})
    endif (WIN32)
    if (${CMAKE_MAJOR_VERSION} GREATER 2 OR ${CMAKE_MINOR_VERSION} GREATER 7)
      set_property(CACHE FORTRAN_NAMING PROPERTY STRINGS ${F2CLIST})
    endif (${CMAKE_MAJOR_VERSION} GREATER 2 OR ${CMAKE_MINOR_VERSION} GREATER 7)
  else (FORTRAN_NAMING STREQUAL "UNKNOWN")
    list(FIND F2CLIST "${FORTRAN_NAMING}" ListIndex)
    if (ListIndex LESS 0)
      message(SEND_ERROR "invalid FORTRAN_NAMING value. Must be empty or one of:
        LOWERCASE
        LOWERCASE_
        LOWERCASE__
        UPPERCASE
        UPPERRCASE_
        UPPERRCASE__")
    endif (ListIndex LESS 0)
  endif (FORTRAN_NAMING STREQUAL "UNKNOWN")

  if (CMAKE_Fortran_COMPILER_ID STREQUAL "GNU" AND CMAKE_Fortran_COMPILER_VERSION VERSION_GREATER_EQUAL 10.0)
    SET (CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -fallow-argument-mismatch")
  endif()

  # Warn about a gfortran 10.2.0 bug (GCC Bug 100149) which
  # causes cg_goto_f to segfault, other versions are fine.
   if (CMAKE_Fortran_COMPILER_ID STREQUAL "GNU" AND 
       CMAKE_Fortran_COMPILER_VERSION VERSION_GREATER "10.1" AND 
       CMAKE_Fortran_COMPILER_VERSION VERSION_LESS "10.3")
    message (WARNING "Fortran mapping (cg_goto_f) is broken for the specified gfortran version.
                           !!! IT'S ADVISABLE TO AVOID VERSION 10.2 !!!")
  endif()

endif (CGNS_ENABLE_FORTRAN)

# this forces cmake to set up the required Fortran variables
if (CGNS_ENABLE_FORTRAN AND HAS_FORTRAN)
  enable_language("Fortran")
endif (CGNS_ENABLE_FORTRAN AND HAS_FORTRAN)

########
# HDF5 #
########

if (FALSE) # XXX(kitware): Hardcoded settings.
option(CGNS_ENABLE_HDF5 "Enable or disable HDF5 interface" "OFF")
else ()
set(CGNS_ENABLE_HDF5 ON)
endif ()

if (FALSE) # XXX(kitware): Adapted for ParaView.
if (CGNS_ENABLE_HDF5)

  if (CGNS_BUILD_SHARED)
    set (FIND_HDF_COMPONENTS C shared)
  else (CGNS_BUILD_SHARED)
    set (FIND_HDF_COMPONENTS C static)
    set(HDF5_USE_STATIC_LIBRARIES ON)
  endif (CGNS_BUILD_SHARED)
  message (STATUS "HDF5 find comps: ${FIND_HDF_COMPONENTS}")

  set (SEARCH_PACKAGE_NAME "hdf5")
  
  # If the first `find_package` below does not succeed, then the legacy `find_package`
  # is tried (the `else` below).  The legacy find_package uses `HDF5_ROOT`.  But if 
  # this is set, then CMake will issue warning and mistakenly say that `HDF5_ROOT` is
  # not used even though it might be.  This can confuse user, so set policy to not
  # issue that warning.
  if (${CMAKE_VERSION} VERSION_GREATER "3.13")
     cmake_policy(SET CMP0074 NEW)
  endif()

  find_package (HDF5 NAMES ${SEARCH_PACKAGE_NAME} COMPONENTS ${FIND_HDF_COMPONENTS})
  message (STATUS "HDF5 C libs:${HDF5_FOUND} static:${HDF5_static_C_FOUND} and shared:${HDF5_shared_C_FOUND}")
  if (HDF5_FOUND)
    if (NOT HDF5_static_C_FOUND AND NOT HDF5_shared_C_FOUND)
      set (FIND_HDF_COMPONENTS C)

      find_package (HDF5 NAMES ${SEARCH_PACKAGE_NAME} COMPONENTS ${FIND_HDF_COMPONENTS})
      message (STATUS "HDF5 libs:${HDF5_FOUND} C:${HDF5_C_FOUND}")
      set (LINK_LIBS ${LINK_LIBS} ${HDF5_LIBRARIES})
      if (HDF5_BUILD_SHARED_LIBS)
        add_definitions (-DH5_BUILT_AS_DYNAMIC_LIB)
      else (HDF5_BUILD_SHARED_LIBS)
        add_definitions (-DH5_BUILT_AS_STATIC_LIB)
      endif (HDF5_BUILD_SHARED_LIBS)
    else (NOT HDF5_static_C_FOUND AND NOT HDF5_shared_C_FOUND)
      if (CGNS_BUILD_SHARED AND HDF5_shared_C_FOUND)
        set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_SHARED_LIBRARY})
      else (CGNS_BUILD_SHARED AND HDF5_shared_C_FOUND)
        set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_STATIC_LIBRARY})
      endif (CGNS_BUILD_SHARED AND HDF5_shared_C_FOUND)
    endif (NOT HDF5_static_C_FOUND AND NOT HDF5_shared_C_FOUND)

  else (HDF5_FOUND)
    find_package (HDF5) # Legacy find
    
    #Legacy find_package does not set HDF5_TOOLS_DIR, so we set it here
    set(HDF5_TOOLS_DIR ${HDF5_LIBRARY}/../bin)
    
    #Legacy find_package does not set HDF5_BUILD_SHARED_LIBS, so we set it here
    if (CGNS_BUILD_SHARED AND EXISTS "${HDF5_LIBRARY}/libhdf5${CMAKE_SHARED_LIBRARY_SUFFIX}")
      set (HDF5_BUILD_SHARED_LIBS 1)
      add_definitions (-DH5_BUILT_AS_DYNAMIC_LIB)
    else ()
      set (HDF5_BUILD_SHARED_LIBS 0)
      add_definitions (-DH5_BUILT_AS_STATIC_LIB)
    endif ()
    set (LINK_LIBS ${LINK_LIBS} ${HDF5_LIBRARIES})

  endif (HDF5_FOUND)
  set (HDF5_PACKAGE_NAME ${SEARCH_PACKAGE_NAME})

  if (HDF5_FOUND)
    if (NOT DEFINED HDF5_INCLUDE_DIRS)
      set (HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIR})
    endif()
    set (HDF5_HAVE_H5PUBCONF_H 1)
    set (HDF5_HAVE_HDF5 1)
    set (HDF5_HDF5_HEADER "h5pubconf.h")
    message (STATUS "HDF5-${HDF5_VERSION} found: INC=${HDF5_INCLUDE_DIRS} TOOLS=${HDF5_TOOLS_DIR} HDF5_BUILD_SHARED_LIBS=${HDF5_BUILD_SHARED_LIBS}")
    INCLUDE_DIRECTORIES (${HDF5_INCLUDE_DIRS})
    set (HDF5_LIBRARY ${LINK_LIBS})
    message (STATUS "HDF5 link libs: ${LINK_LIBS}")
  else (HDF5_FOUND)
    message (FATAL_ERROR " HDF5 was specified but was not found")
  endif (HDF5_FOUND)

  set(HDF5_NEED_ZLIB "OFF" CACHE BOOL "Does the HDF5 library require linking to zlib?")
  if(HDF5_NEED_ZLIB)
    find_library(ZLIB_LIBRARY z)
    mark_as_advanced(CLEAR ZLIB_LIBRARY)
  else (HDF5_NEED_ZLIB)
    mark_as_advanced(FORCE ZLIB_LIBRARY)
  endif(HDF5_NEED_ZLIB)

  set(HDF5_NEED_SZIP "OFF" CACHE BOOL "Does the HDF5 library require linking to szip?")
  if (HDF5_NEED_SZIP)
    find_library(SZIP_LIBRARY szip)
    mark_as_advanced(CLEAR SZIP_LIBRARY)
  else (HDF5_NEED_SZIP)
    mark_as_advanced(FORCE SZIP_LIBRARY)
  endif (HDF5_NEED_SZIP)

  # Check if HDF5 version is 1.8 or greater
  if (HDF5_VERSION VERSION_LESS "1.8.0")
    message (FATAL_ERROR "HDF5 version must be greater than 1.8.0")
  endif (HDF5_VERSION VERSION_LESS "1.8.0")

  set(HDF5_NEED_MPI "OFF" CACHE BOOL "Does the HDF5 library require linking to mpi? (Only true if using parallel HDF5)")
  set(MPI_INC)
  set(MPI_LIBS)
  if (HDF5_NEED_MPI)
    if (NOT MPI_FOUND)
      include(FindMPI)
    endif (NOT MPI_FOUND)
    # MPI variable names changed with cmake 2.8.5
    if (DEFINED MPI_C_COMPILER)
      mark_as_advanced(CLEAR MPI_C_COMPILER MPI_C_INCLUDE_PATH MPI_C_LIBRARIES)
      mark_as_advanced(FORCE MPI_LIBRARY MPI_EXTRA_LIBRARY)
      set(MPI_INC ${MPI_C_INCLUDE_PATH})
      set(MPI_LIBS ${MPI_C_LIBRARIES})
    else (DEFINED MPI_C_COMPILER)
      mark_as_advanced(CLEAR MPI_COMPILER MPI_INCLUDE_PATH MPI_LIBRARY MPI_EXTRA_LIBRARY)
      set(MPI_INC ${MPI_INCLUDE_PATH})
      if (MPI_EXTRA_LIBRARY)
        set(MPI_LIBS ${MPI_LIBRARY} ${MPI_EXTRA_LIBRARY})
      else (MPI_EXTRA_LIBRARY)
        set(MPI_LIBS ${MPI_LIBRARY})
      endif (MPI_EXTRA_LIBRARY)
    endif (DEFINED MPI_C_COMPILER)
    mark_as_advanced(CLEAR MPIEXEC)
  else (HDF5_NEED_MPI)
    mark_as_advanced(FORCE MPI_C_COMPILER MPI_C_INCLUDE_PATH MPI_C_LIBRARIES)
    mark_as_advanced(FORCE MPI_COMPILER MPI_INCLUDE_PATH MPI_LIBRARY MPI_EXTRA_LIBRARY MPIEXEC)
  endif (HDF5_NEED_MPI)
  mark_as_advanced(CLEAR HDF5_NEED_ZLIB HDF5_NEED_SZIP HDF5_NEED_MPI)
else (CGNS_ENABLE_HDF5)
  mark_as_advanced(FORCE HDF5_NEED_ZLIB HDF5_NEED_SZIP HDF5_NEED_MPI)
  mark_as_advanced(FORCE ZLIB_LIBRARY SZIP_LIBRARY)
  mark_as_advanced(FORCE MPI_C_COMPILER MPI_C_INCLUDE_PATH MPI_C_LIBRARIES)
  mark_as_advanced(FORCE MPI_COMPILER MPI_INCLUDE_PATH MPI_LIBRARY MPI_EXTRA_LIBRARY MPIEXEC)
endif (CGNS_ENABLE_HDF5)
else ()
if (VTK_MODULE_USE_EXTERNAL_VTK_hdf5)
  # TODO: Implement.
  set(H5_HAVE_PARALLEL OFF)
else ()
  set(H5_HAVE_PARALLEL OFF)
endif ()
endif ()

#################
# Parallel CGNS #
#################

if (FALSE) # XXX(kitware): Adapted for ParaView.
if (CGNS_ENABLE_HDF5 AND HDF5_NEED_MPI)
  set(CGNS_ENABLE_PARALLEL "OFF" CACHE BOOL "Enable or disable parallel interface ?")
  mark_as_advanced(CLEAR CGNS_ENABLE_PARALLEL)
  # Check that HDF5 has parallel support
  if (NOT (HDF5_IS_PARALLEL OR HDF5_ENABLE_PARALLEL))
    message(FATAL_ERROR "HDF5 has been found, but is missing parallel support.")
  endif()
else (CGNS_ENABLE_HDF5 AND HDF5_NEED_MPI)
  set(CGNS_ENABLE_PARALLEL "OFF")
  mark_as_advanced(FORCE CGNS_ENABLE_PARALLEL)
endif (CGNS_ENABLE_HDF5 AND HDF5_NEED_MPI)
else ()
set(CGNS_ENABLE_PARALLEL ${H5_HAVE_PARALLEL})
endif ()

####################
# RPATH Management #
####################

if (FALSE) # XXX(kitware): ParaView handles rpaths.
if (CGNS_BUILD_SHARED)
  # use, i.e. don't skip the full RPATH for the build tree
  set(CMAKE_SKIP_BUILD_RPATH  FALSE)

  # when building, don't use the install RPATH already
  # (but later on when installing)
  set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

  # the RPATH to be used when installing
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

  IF (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH TRUE)
  ENDIF()
else(CGNS_BUILD_SHARED)
  set(CMAKE_SKIP_RPATH TRUE)
endif(CGNS_BUILD_SHARED)
endif ()

#-----------------------------------------------------------------------------
# Dashboard and Testing Settings
#-----------------------------------------------------------------------------
if (FALSE) # XXX(kitware): Hardcoded settings.
option (CGNS_BUILD_TESTING "Build CGNS Testing" OFF)
else ()
set(CGNS_BUILD_TESTING OFF)
endif ()
if (CGNS_BUILD_TESTING)
  set (DART_TESTING_TIMEOUT "1200"
      CACHE STRING
      "Timeout in seconds for each test (default 1200=20minutes)"
  )
  enable_testing ()
  include (CTest)
  include (${PROJECT_SOURCE_DIR}/CTestConfig.cmake)
endif (CGNS_BUILD_TESTING)

########
# CGNS #
########

# Include the src directory
add_subdirectory(src)
