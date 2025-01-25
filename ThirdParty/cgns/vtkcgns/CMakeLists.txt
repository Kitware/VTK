if (FALSE) # XXX(kitware): VTK handles versions.
cmake_minimum_required(VERSION 3.20)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.10")
  cmake_policy(SET CMP0015 NEW)
endif()
if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13")
  cmake_policy(SET CMP0081 NEW)
endif()

# Macro to handle muticonfig generator
macro (SET_CGNS_BUILD_TYPE)
  get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(_isMultiConfig)
    set(CGNS_CFG_NAME ${CMAKE_BUILD_TYPE})
    set(CGNS_BUILD_TYPE ${CMAKE_CFG_INTDIR})
    set(CGNS_CFG_BUILD_TYPE \${CMAKE_INSTALL_CONFIG_NAME})
  else()
    set(CGNS_CFG_BUILD_TYPE ".")
    if(CMAKE_BUILD_TYPE)
      set(CGNS_CFG_NAME ${CMAKE_BUILD_TYPE})
      set(CGNS_BUILD_TYPE ${CMAKE_BUILD_TYPE})
    else()
      set(CGNS_CFG_NAME "Release")
      set(CGNS_BUILD_TYPE "Release")
    endif()
  endif()
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15.0")
      message (VERBOSE "Setting build type to 'Release' as none was specified.")
    endif()
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
      "MinSizeRel" "RelWithDebInfo")
  endif()
endmacro ()

SET_CGNS_BUILD_TYPE()
endif ()

project("cgns" C)
if (FALSE) # XXX(kitware): VTK handles install directories.
if (WIN32)
  set(${CMAKE_INSTALL_LIBDIR} "lib")
  set(${CMAKE_INSTALL_DATADIR} "share")
  set(${CMAKE_INSTALL_INCLUDEDIR} "include")
  set(${CMAKE_INSTALL_BINDIR} "bin")
  message(STATUS "Setting installation destination on Windows to: ${CMAKE_INSTALL_PREFIX}")
else()
  include(GNUInstallDirs)
endif()
endif ()

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
endif ()
endif ()

##############
# build mode #
##############

include(CheckIncludeFile)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(inttypes.h HAVE_INTTYPES_H)
if (NOT HAVE_STDINT_H)
  message(FATAL_ERROR "stdint.h is not found")
endif ()
if (NOT HAVE_INTTYPES_H)
  message(FATAL_ERROR "inttypes.h is not found")
endif ()

# get some type sizes
include(CheckTypeSize)
CHECK_TYPE_SIZE(long SIZEOF_LONG)
CHECK_TYPE_SIZE(off_t SIZEOF_OFF_T)
CHECK_TYPE_SIZE("int64_t" SIZEOF_INT64_T)
CHECK_TYPE_SIZE("long long" SIZEOF_LONG_LONG)
if (FALSE) # XXX(kitware): Hardcoded settings.

if ("${SIZEOF_OFF_T}" LESS 8)
  option(CGNS_ENABLE_LFS "Enable or disable large file support" "OFF")
endif ()

if ("${CMAKE_SIZEOF_VOID_P}" GREATER 4)
  option(CGNS_ENABLE_64BIT "Enable or disable 64-bit code building" "ON")
else ()
  set(CGNS_ENABLE_64BIT "OFF")
endif ()

option(CGNS_ENABLE_LEGACY "Enable or disable building legacy code (3.0 compatible)" "OFF")
option(CGNS_ENABLE_SCOPING "Enable or disable scoping of enumeration values" "OFF")
option(CGNS_ENABLE_BASE_SCOPE "Enable or disable base scoped families or connectivities" "OFF")
option(CGNS_ENABLE_MEM_DEBUG "Enable or disable memory debugging" "OFF")

set(CGNS_BUILD_SHARED "ON" CACHE BOOL "Build a shared version of the library")
else ()
if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CGNS_ENABLE_64BIT ON)
  set(CGNS_ENABLE_LFS OFF) # only needed on 32bit systems
else ()
  set(CGNS_ENABLE_64BIT OFF)
  set(CGNS_ENABLE_LFS ON)
endif ()
set(CGNS_ENABLE_LEGACY OFF)
set(CGNS_ENABLE_SCOPING ON)
set(CGNS_ENABLE_BASE_SCOPE OFF)
set(CGNS_ENABLE_MEM_DEBUG OFF)
endif ()


if (CGNS_ENABLE_LEGACY)
  set(CGNS_ENABLE_64BIT "OFF")
  mark_as_advanced(FORCE CGNS_ENABLE_64BIT)
else ()
  mark_as_advanced(CLEAR CGNS_ENABLE_64BIT)
endif ()

if (CGNS_ENABLE_64BIT)
  set(CGNS_ENABLE_LEGACY "OFF")
  mark_as_advanced(FORCE CGNS_ENABLE_LEGACY)
else ()
  mark_as_advanced(CLEAR CGNS_ENABLE_LEGACY)
endif ()
	
if (CGNS_ENABLE_LFS)
  include(CheckSymbolExists)
  if (WIN32)
    # Windows does not have an _open64 function
    set(HAVE_OPEN64 0)
    check_symbol_exists(_lseeki64 "io.h" HAVE_LSEEK64)
  else ()
    check_symbol_exists(open64 "sys/types.h;sys/stat.h;unistd.h" HAVE_OPEN64)
    check_symbol_exists(lseek64 "sys/types.h;unistd.h" HAVE_LSEEK64)
  endif ()
  if (HAVE_OPEN64)
    add_compile_definitions(HAVE_OPEN64)
  endif ()
  if (HAVE_LSEEK64)
    add_compile_definitions(HAVE_LSEEK64)
  endif ()
else ()
  remove_definitions(-DHAVE_OPEN64 -DHAVE_LSEEK64)
endif ()

# XXX(kitware): Hardcoded for ParaView.
if (FALSE)
option(CGNS_ENABLE_TESTS "Enable test programs" "OFF")
if (CGNS_ENABLE_TESTS)
  enable_testing()
endif ()
else ()
set(CGNS_ENABLE_TESTS OFF)
endif ()

###########
# Fortran #
###########

if (FALSE) # XXX(kitware): Hardcoded settings.
# Allow fortran support to be toggled
option(CGNS_ENABLE_FORTRAN "Enable or disable the use of Fortran" "OFF")
if (NOT DEFINED FORTRAN_NAMING)
  set(FORTRAN_NAMING "UNKNOWN")
endif ()
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
    else ()
      set(FORTRAN_NAMING "LOWERCASE_" CACHE STRING ${FORTRAN_NAMING_HELP})
    endif ()
    if (${CMAKE_MAJOR_VERSION} GREATER 2 OR ${CMAKE_MINOR_VERSION} GREATER 7)
      set_property(CACHE FORTRAN_NAMING PROPERTY STRINGS ${F2CLIST})
    endif (${CMAKE_MAJOR_VERSION} GREATER 2 OR ${CMAKE_MINOR_VERSION} GREATER 7)
  else ()
    list(FIND F2CLIST "${FORTRAN_NAMING}" ListIndex)
    if (ListIndex LESS 0)
      message(SEND_ERROR "invalid FORTRAN_NAMING value. Must be empty or one of:
        LOWERCASE
        LOWERCASE_
        LOWERCASE__
        UPPERCASE
        UPPERRCASE_
        UPPERRCASE__")
    endif ()
  endif ()

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
endif ()

########
# HDF5 #
########

if (FALSE) # XXX(kitware): Hardcoded settings.
option(CGNS_ENABLE_HDF5 "Enable or disable HDF5 interface" "ON")
else ()
set(CGNS_ENABLE_HDF5 ON)
endif ()

if (FALSE) # XXX(kitware): Adapted for ParaView.
if (CGNS_ENABLE_HDF5)

  if (CGNS_BUILD_SHARED)
    set (CG_HDF5_LINK_TYPE "shared")
    set (CG_HDF5_LINK_TYPE_UPPER "SHARED")
  else ()
    set (CG_HDF5_LINK_TYPE "static")
    set (CG_HDF5_LINK_TYPE_UPPER "STATIC")
    set(HDF5_USE_STATIC_LIBRARIES ON)
  endif ()
  set (FIND_HDF_COMPONENTS C ${CG_HDF5_LINK_TYPE})
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
      if (CG_HDF5_LINK_TYPE STREQUAL "shared")
        set(CG_HDF5_LINK_TYPE "static")
        set(CG_HDF5_LINK_TYPE_UPPER "STATIC")
      else()
        set(CG_HDF5_LINK_TYPE "shared")
        set(CG_HDF5_LINK_TYPE_UPPER "SHARED")
      endif()

      set (FIND_HDF_COMPONENTS C ${CG_HDF5_LINK_TYPE})

      find_package (HDF5 NAMES ${SEARCH_PACKAGE_NAME} COMPONENTS ${FIND_HDF_COMPONENTS} REQUIRED)
      message (STATUS "HDF5 libs:${HDF5_FOUND} C:${HDF5_C_${CG_HDF5_LINK_TYPE_UPPER}_LIBRARY}")
    endif()
 
    set(LINK_LIBS ${HDF5_C_${CG_HDF5_LINK_TYPE_UPPER}_LIBRARY})
  else ()
    find_package (HDF5) # Legacy find
    
    #Legacy find_package does not set HDF5_TOOLS_DIR, so we set it here
    set(HDF5_TOOLS_DIR ${HDF5_LIBRARY}/../bin)
    
    #Legacy find_package does not set HDF5_BUILD_SHARED_LIBS, so we set it here
    set(GUESS_SHARED "${HDF5_LIBRARY}/libhdf5${CMAKE_SHARED_LIBRARY_SUFFIX}")
    foreach (ITEM ${HDF5_LIBRARIES})
      if (ITEM MATCHES "(.*)hdf5${CMAKE_SHARED_LIBRARY_SUFFIX}")
        set(GUESS_SHARED "${ITEM}")
        break()
      endif()
    endforeach()
    if (CGNS_BUILD_SHARED AND EXISTS "${GUESS_SHARED}")
      set (HDF5_BUILD_SHARED_LIBS 1)
      set (CG_HDF5_LINK_TYPE "shared")
      set (CG_HDF5_LINK_TYPE_UPPER "SHARED")
    else ()
      set (HDF5_BUILD_SHARED_LIBS 0)
      set (CG_HDF5_LINK_TYPE "static")
      set (CG_HDF5_LINK_TYPE_UPPER "STATIC")
    endif ()
    set (LINK_LIBS ${HDF5_LIBRARIES})
  endif ()
  set (HDF5_PACKAGE_NAME ${SEARCH_PACKAGE_NAME})

  if (HDF5_FOUND)
    if (NOT DEFINED HDF5_INCLUDE_DIRS)
      set (HDF5_INCLUDE_DIRS ${HDF5_INCLUDE_DIR})
    endif()
    set (HDF5_HAVE_H5PUBCONF_H 1)
    set (HDF5_HAVE_HDF5 1)
    set (HDF5_HDF5_HEADER "h5pubconf.h")
    message (STATUS "HDF5-${HDF5_VERSION} found: INC=${HDF5_INCLUDE_DIRS} TOOLS=${HDF5_TOOLS_DIR} HDF5_BUILD_SHARED_LIBS=${HDF5_BUILD_SHARED_LIBS}")
    set (HDF5_LIBRARY ${LINK_LIBS})
    message (STATUS "HDF5 link libs: ${LINK_LIBS}")
  else ()
    message (FATAL_ERROR " HDF5 was specified but was not found")
  endif ()

  set(HDF5_NEED_ZLIB "OFF" CACHE BOOL "Does the HDF5 library require linking to zlib?")
  if(HDF5_NEED_ZLIB)
    find_library(ZLIB_LIBRARY z)
    mark_as_advanced(CLEAR ZLIB_LIBRARY)
  else ()
    mark_as_advanced(FORCE ZLIB_LIBRARY)
  endif()

  set(HDF5_NEED_SZIP "OFF" CACHE BOOL "Does the HDF5 library require linking to szip?")
  if (HDF5_NEED_SZIP)
    find_library(SZIP_LIBRARY NAMES szip sz)
    mark_as_advanced(CLEAR SZIP_LIBRARY)
  else ()
    mark_as_advanced(FORCE SZIP_LIBRARY)
  endif ()

  # Check if HDF5 version is 1.8 or greater
  if (HDF5_VERSION VERSION_LESS "1.8.0")
    message (FATAL_ERROR "HDF5 ${HDF5_VERSION}: version must be greater than 1.8.0")
  endif ()

  set(HDF5_NEED_MPI "OFF" CACHE BOOL "Does the HDF5 library require linking to mpi? (Only true if using parallel HDF5)")
  set(MPI_INC)
  set(MPI_LIBS)
  if (HDF5_NEED_MPI)
    if (NOT MPI_FOUND)
      include(FindMPI)
    endif ()
    # MPI variable names changed with cmake 2.8.5
    if (DEFINED MPI_C_COMPILER)
      mark_as_advanced(CLEAR MPI_C_COMPILER MPI_C_INCLUDE_PATH MPI_C_LIBRARIES)
      mark_as_advanced(FORCE MPI_LIBRARY MPI_EXTRA_LIBRARY)
      set(MPI_INC ${MPI_C_INCLUDE_PATH})
      set(MPI_LIBS ${MPI_C_LIBRARIES})
    else ()
      mark_as_advanced(CLEAR MPI_COMPILER MPI_INCLUDE_PATH MPI_LIBRARY MPI_EXTRA_LIBRARY)
      set(MPI_INC ${MPI_INCLUDE_PATH})
      if (MPI_EXTRA_LIBRARY)
        set(MPI_LIBS ${MPI_LIBRARY} ${MPI_EXTRA_LIBRARY})
      else ()
        set(MPI_LIBS ${MPI_LIBRARY})
      endif ()
    endif ()
    mark_as_advanced(CLEAR MPIEXEC)
  else ()
    mark_as_advanced(FORCE MPI_C_COMPILER MPI_C_INCLUDE_PATH MPI_C_LIBRARIES)
    mark_as_advanced(FORCE MPI_COMPILER MPI_INCLUDE_PATH MPI_LIBRARY MPI_EXTRA_LIBRARY MPIEXEC)
  endif ()
  mark_as_advanced(CLEAR HDF5_NEED_ZLIB HDF5_NEED_SZIP HDF5_NEED_MPI)

  #Modern Target Library import if not defined
  if (NOT TARGET hdf5-${CG_HDF5_LINK_TYPE})
    add_library(hdf5-${CG_HDF5_LINK_TYPE} INTERFACE IMPORTED)
    string(REPLACE "-D" "" _hdf5_definitions "${HDF5_DEFINITIONS}")
    set_target_properties(hdf5-${CG_HDF5_LINK_TYPE} PROPERTIES
                          INTERFACE_LINK_LIBRARIES "${HDF5_LIBRARY}"
                          INTERFACE_INCLUDE_DIRECTORIES "${HDF5_INCLUDE_DIRS}"
                          INTERFACE_COMPILE_DEFINITIONS "${_hdf5_definitions}")
    if (CG_HDF5_LINK_TYPE STREQUAL "shared")
      set_target_properties(hdf5-${CG_HDF5_LINK_TYPE} PROPERTIES
                            INTERFACE_COMPILE_DEFINITIONS H5_BUILT_AS_DYNAMIC_LIB)

    else()
      set_target_properties(hdf5-${CG_HDF5_LINK_TYPE} PROPERTIES
			    INTERFACE_COMPILE_DEFINITIONS H5_BUILT_AS_STATIC_LIB)
    endif()
  endif ()
else ()
  mark_as_advanced(FORCE HDF5_NEED_ZLIB HDF5_NEED_SZIP HDF5_NEED_MPI)
  mark_as_advanced(FORCE ZLIB_LIBRARY SZIP_LIBRARY)
  mark_as_advanced(FORCE MPI_C_COMPILER MPI_C_INCLUDE_PATH MPI_C_LIBRARIES)
  mark_as_advanced(FORCE MPI_COMPILER MPI_INCLUDE_PATH MPI_LIBRARY MPI_EXTRA_LIBRARY MPIEXEC)
endif ()
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
else ()
  set(CGNS_ENABLE_PARALLEL "OFF")
  mark_as_advanced(FORCE CGNS_ENABLE_PARALLEL)
endif ()
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
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}; ${CMAKE_INSTALL_PREFIX}/lib")

  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_MACOSX_RPATH TRUE)
  endif()
else()
  set(CMAKE_SKIP_RPATH TRUE)
endif()
endif ()

#-----------------------------------------------------------------------------
# Dashboard and Testing Settings
#-----------------------------------------------------------------------------
option (CGNS_BUILD_TESTING "Build CGNS Testing" OFF)
if (CGNS_BUILD_TESTING)
  set (DART_TESTING_TIMEOUT "600"
      CACHE STRING
      "Timeout in seconds for each test (default 600=10minutes)"
  )
  enable_testing ()
  include (CTest)
  include (${PROJECT_SOURCE_DIR}/CTestConfig.cmake)
endif ()

########
# CGNS #
########

# Include the src directory
add_subdirectory(src)
