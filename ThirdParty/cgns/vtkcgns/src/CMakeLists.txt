
##########
# Global #
##########

if (WIN32)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

################################
# create configured header files #
################################

if("${SIZEOF_INT64_T}" EQUAL "8")
  set(CGLONGT "int64_t")
  set(CGULONGT "uint64_t")
elseif("${SIZEOF_LONG}" EQUAL "8")
  set(CGLONGT "long")
  set(CGULONGT "unsigned long")
elseif("${SIZEOF_LONG_LONG}" EQUAL "8")
  set(CGLONGT "long long")
  set(CGULONGT "unsigned long long")
else()
  message(FATAL_ERROR "Can't find suitable int64_t")
endif()
message(STATUS "Found int64_t: ${CGLONGT}")

set(BUILDLEGACY 0)
set(BUILD64BIT 0)
set(BUILDSCOPE 0)
set(BUILDFORTRAN 0)
set(BUILDBASESCOPE 0)

if (CGNS_ENABLE_LEGACY)
  set(BUILDLEGACY 1)
else ()
  if (CGNS_ENABLE_64BIT)
    set(BUILD64BIT 1)
  else ()
  endif ()
endif ()
if (CGNS_ENABLE_SCOPING)
  set(BUILDSCOPE 1)
endif ()
if (CGNS_ENABLE_FORTRAN)
  set(BUILDFORTRAN 1)
endif ()
if (CGNS_ENABLE_BASE_SCOPE)
  set(BUILDBASESCOPE 1)
endif ()

if (FALSE) # XXX(kitware): VTK handles flags better.
set(SYSCFLAGS "")
set(CFGFLAGS "")
set(LIBS "")

file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/lib LIBDIR)
file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/include INCLUDEDIR)
file(TO_NATIVE_PATH ${CMAKE_C_COMPILER} CC)
if (WIN32)
  set(CGNSLIB cgns.lib)
else ()
  set(CGNSLIB libcgns.a)
  set(LIBS -lm)
endif ()
endif ()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if (FALSE) # XXX(kitware): VTK handles flags better.
include(CheckCCompilerFlag)
macro(check_and_add_compiler_option _option)
  message(STATUS "Checking C compiler flag ${_option}")
  string(REPLACE "=" "-" _temp_option_variable ${_option})
  string(REGEX REPLACE "^-" "" _option_variable ${_temp_option_variable})
  check_c_compiler_flag("${_option}" ${_option_variable})
  if(${${_option_variable}})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_option}")
  endif()
endmacro()
if (CMAKE_C_COMPILER_ID MATCHES "GNU")
  check_and_add_compiler_option("-std=gnu99")
elseif (CMAKE_C_COMPILER_ID MATCHES "XL")
  check_and_add_compiler_option("-qlanglvl=extc99")
elseif(CMAKE_C_COMPILER_ID MATCHES "Sun")
  check_and_add_compiler_option("-xc99")
endif()
endif ()

if (NOT DEFINED CMAKE_CONFIGURATION_TYPES)
  # Buggy section used for generating buildConfig.def
  # A CMake generator with conditional could be used
  # as using CMAKE_BUILD_TYPE is not good practise
  string(COMPARE EQUAL ${CMAKE_BUILD_TYPE} Debug IS_DEBUG)
  if (IS_DEBUG)
    set(BUILDDEBUG 1)
    set(CFLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
    set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
  else ()
    set(BUILDDEBUG 0)
    set(CFLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
    set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
  endif ()
else ()
  set(BUILDDEBUG 0)
  set(CFLAGS "")
  set(LDFLAGS "")
endif ()

set(BUILDHDF5 0)
set(H5NEEDMPI 0)
set(H5NEEDSZIP 0)
set(H5NEEDZLIB 0)
set(BUILDPARALLEL 0)
set(HDF5INC "")
set(HDF5LIB "")
set(SZIPLIB "")
set(ZLIBLIB "")
set(MPIINC "")
set(MPILIBS "")
set(HAVE_FORTRAN_95 0)
set(HAVE_FORTRAN_2003 0)
set(HAVE_FORTRAN_2008 0)
set(HAVE_FORTRAN_2008TS 0)
set(HDF5_HAVE_MULTI_DATASETS 0)
set(HDF5_HAVE_COLL_METADATA 0)
set(HDF5_HAVE_FILE_SPACE_STRATEGY 0)
set(FORTRAN_DEFAULT_INTEGER_C_INT64_T 0)
set(HAVE_STAT64_STRUCT 0)
set(HAVE_COMPLEX_C99_EXT 0)

#For cg_hash_types.h.in
CHECK_INCLUDE_FILE(stdint.h HAVE_STDINT_H)
set(SIZEOF_VOID_P ${CMAKE_SIZEOF_VOID_P})

include(CheckCSourceCompiles)
check_c_source_compiles("
#include <complex.h>
int main(int argc, char *argv[]){
#if !defined(_MSC_VER)
float _Complex a[2];
__real__(a[1]) = (float) 1.0;
__imag__(a[1]) = (float) 0.0;
#endif
return 0;
}
"
  HAVE_COMPLEX_C99_EXT
)

if (BUILDFORTRAN)

#----------- Determine if the Fortran compiler pgi ------------------------
# The PGI compiler segfaults when va_start is called, so we need to add
# a special compiler flag, see CGNS-40.
#--------------------------------------------------------------------------
if(CMAKE_Fortran_COMPILER_ID MATCHES "PGI")
 SET(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Mx,125,0x200")
endif()

#----------- Determine if the Fortran compiler NAG ------------------------
# The NAG compiler needs an additional flag for Fortran calls to C APIs.
#--------------------------------------------------------------------------
if(CMAKE_Fortran_COMPILER_ID MATCHES "NAG")
 SET(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -mismatch_all")
 if ("${HDF5_VERSION}" VERSION_GREATER_EQUAL "1.14.3")
    SET(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -ieee=full")
 endif()
endif()

ENABLE_LANGUAGE(Fortran)

#-----------------------------------------------------------------------------
# The provided CMake Fortran macros don't provide a general check function
# so use this one.
#-----------------------------------------------------------------------------
macro (CHECK_FORTRAN_FEATURE FUNCTION CODE VARIABLE)
  if(NOT DEFINED ${VARIABLE})
    if(CMAKE_REQUIRED_LIBRARIES)
      set (CHECK_FUNCTION_EXISTS_ADD_LIBRARIES
          "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    else()
      set (CHECK_FUNCTION_EXISTS_ADD_LIBRARIES)
    endif()
    file (WRITE
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.F90
        "${CODE}"
    )
    try_compile (${VARIABLE}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.F90
        CMAKE_FLAGS "${CHECK_FUNCTION_EXISTS_ADD_LIBRARIES}"
        OUTPUT_VARIABLE OUTPUT
    )

#    message ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
#    message ( "Test result ${OUTPUT}")
#    message ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")

    if(${VARIABLE})
      set (${VARIABLE} 1 CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      message (STATUS "Testing Fortran ${FUNCTION} - OK")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the Fortran ${FUNCTION} exists passed with the following output:\n"
          "${OUTPUT}\n\n"
      )
    else()
      message (STATUS "Testing Fortran ${FUNCTION} - Fail")
      set (${VARIABLE} "" CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the Fortran ${FUNCTION} exists failed with the following output:\n"
          "${OUTPUT}\n\n")
    endif()
  endif()
endmacro (CHECK_FORTRAN_FEATURE)

#-----------------------------------------------------------------------------
# Configure Checks which require Fortran compilation must go in here
# not in the main ConfigureChecks.cmake files, because if the user has
# no Fortran compiler, problems arise.
#
#-----------------------------------------------------------------------------

CHECK_FORTRAN_FEATURE(Fortran2003
  "
       PROGRAM testf03
  	 USE iso_c_binding
  	 IMPLICIT NONE
  	 INTEGER(C_INT) :: a
  	 INTEGER(C_INT32_T) :: b
  	 INTEGER(C_INT64_T) :: c
         REAL(C_FLOAT) :: d
  	 REAL(C_DOUBLE) :: e
  	 CHARACTER(LEN=1,KIND=C_CHAR), TARGET :: f
  	 TYPE(C_PTR) :: g
         INTERFACE 
           INTEGER(C_INT) FUNCTION test(A, UserDataName1) BIND(C, name='cg_goto')
             USE ISO_C_BINDING
             INTEGER(C_INT)   , INTENT(IN), VALUE :: A
             CHARACTER(C_CHAR), DIMENSION(*), INTENT(IN) :: UserDataName1
           END FUNCTION test
         END INTERFACE
  	 g = C_LOC(f(1:1))
      END PROGRAM testf03 
  "
  CHECK_FORTRAN_2003
)
if (CHECK_FORTRAN_2003)
  set(HAVE_FORTRAN_2003 1)
endif ()
   

if (HAVE_FORTRAN_2003)
CHECK_FORTRAN_FEATURE(Fortran2008TS
  " 
       PROGRAM testf03
         USE ISO_C_BINDING
         INTERFACE 
            INTEGER(C_INT) FUNCTION test(A) BIND(C, name='test')
              USE ISO_C_BINDING
              INTEGER(C_INT), OPTIONAL :: A
            END FUNCTION test
         END INTERFACE
       END PROGRAM testf03 
  "
  CHECK_FORTRAN_2008TS
)
if (CHECK_FORTRAN_2008TS)
  set(HAVE_FORTRAN_2008TS 1)
endif (CHECK_FORTRAN_2008TS)

CHECK_FORTRAN_FEATURE(Fortran2008
  " 
       MODULE test
         USE ISO_C_BINDING
         INTERFACE binding
           FUNCTION bind1(x) BIND(C, name='Cfunc')
             IMPORT :: C_FLOAT
             REAL(C_FLOAT) :: x
           END FUNCTION bind1
           FUNCTION bind2(x) BIND(C, name='Cfunc')
             IMPORT :: C_DOUBLE
             REAL(C_DOUBLE) :: x
           END FUNCTION bind2
         END INTERFACE
       END MODULE test

       PROGRAM main
         USE test
         IMPLICIT NONE
       END PROGRAM
  "
  CHECK_FORTRAN_2008
)
if (CHECK_FORTRAN_2008)
  set(HAVE_FORTRAN_2008 1)
endif ()

CHECK_FORTRAN_FEATURE(Sizeof_int
  "
      MODULE mod

        INTERFACE test_integer
           MODULE PROCEDURE test_int
           MODULE PROCEDURE test_c_int64
        END INTERFACE
        
      CONTAINS
        
        SUBROUTINE test_c_int64(a)
          USE ISO_C_BINDING
          INTEGER(C_INT64_T) :: a
        END SUBROUTINE test_c_int64
        
        SUBROUTINE test_int(a)
          USE ISO_C_BINDING
          INTEGER :: a
        END SUBROUTINE test_int
        
      END MODULE mod

      PROGRAM main
        
        USE mod
        INTEGER :: a
        
        CALL test_integer(a)
        
      END PROGRAM main
  "
  CHECK_FORTRAN_DEFAULT_INTEGER_C_INT32_T
)

if (NOT CHECK_FORTRAN_DEFAULT_INTEGER_C_INT32_T)
  set(FORTRAN_DEFAULT_INTEGER_C_INT64_T 1)
endif ()

else ()

CHECK_FORTRAN_FEATURE(Fortran95
  " 
      PROGRAM testf95
  	 IMPLICIT NONE
         INTEGER, PARAMETER :: cgint_kind = SELECTED_INT_KIND(5)
      END PROGRAM testf95
  "
  CHECK_FORTRAN_95
)

if (CHECK_FORTRAN_95)
  set(HAVE_FORTRAN_95 1)
endif ()
endif ()

CHECK_FORTRAN_FEATURE(FortranConcat
  "
# define CONCATENATE(a,b) a##b
      CONCATENATE(PRO,GRAM) testconcat
         IMPLICIT NONE
          integer a
          a = 10
      END PROGRAM testconcat
  "
  CHECK_FORTRAN_CONCAT
)
if (NOT CHECK_FORTRAN_CONCAT)
 SET(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -DNO_CONCATENATION")
endif ()

endif ()

macro(native_paths INPUT_PATH_VARIABLE RESULT_VARIABLE)
  set(${RESULT_VARIABLE} "")
  if (FALSE) # XXX(kitware): This hard-codes full paths into the build
  foreach(PATH ${${INPUT_PATH_VARIABLE}})
    file(TO_NATIVE_PATH ${PATH} NATIVE_PATH)
    list(APPEND ${RESULT_VARIABLE} ${NATIVE_PATH})
  endforeach()
  endif ()
endmacro()

#-----------------------------------------------------------------------------
# The provided CMake macros don't provide a general HDF5 check function
# so use this one.
#-----------------------------------------------------------------------------

macro (CHECK_HDF5_FEATURE FUNCTION CODE VARIABLE)
  if(NOT DEFINED ${VARIABLE})
    file (WRITE
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_HDF.c
        "${CODE}"
    )
    set (CHECK_FUNCTION_EXISTS_ADD_INCLUDE "-DINCLUDE_DIRECTORIES:STRING=${HDF5_INCLUDE_DIR}")
    try_compile (${VARIABLE}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_HDF.c
        LINK_LIBRARIES ${HDF5_LIBRARY}
        CMAKE_FLAGS "${CHECK_FUNCTION_EXISTS_ADD_INCLUDE}"
        OUTPUT_VARIABLE OUTPUT
    )

#    message ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
#    message ( "Test result ${OUTPUT}")
#    message ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")

    if(${VARIABLE})
      set (${VARIABLE} 1 CACHE INTERNAL "Have HDF5 function ${FUNCTION}")
      message (STATUS "Testing HDF5 ${FUNCTION} - OK")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the HDF5 ${FUNCTION} exists passed with the following output:\n"
          "${OUTPUT}\n\n"
      )
    else()
      message (STATUS "Testing HDF5 ${FUNCTION} - NOT FOUND")
      set (${VARIABLE} "" CACHE INTERNAL "Have HDF5 function ${FUNCTION}")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the HDF5 ${FUNCTION} exists failed with the following output:\n"
          "${OUTPUT}\n\n")
    endif()
  endif()
endmacro ()

if (CGNS_ENABLE_HDF5)
  set(BUILDHDF5 1)
  if (HDF5_INCLUDE_PATH)
    native_paths(HDF5_INCLUDE_PATH HDF5INC)
  endif ()
  if (HDF5_LIBRARY)
    native_paths(HDF5_LIBRARY HDF5LIB)
  endif ()
  if (HDF5_NEED_SZIP)
    set(H5NEEDSZIP 1)
    if (SZIP_LIBRARY)
      native_paths(SZIP_LIBRARY SZIPLIB)
    endif ()
  endif ()
  if (HDF5_NEED_ZLIB)
    set(H5NEEDZLIB 1)
    if (ZLIB_LIBRARY)
      native_paths(ZLIB_LIBRARY ZLIBLIB)
    endif ()
  endif ()
  if (HDF5_NEED_MPI)
    set(H5NEEDMPI 1)
    if (MPI_INC)
      native_paths(MPI_INC MPIINC)
    endif ()
    if (MPI_LIBS)
      native_paths(MPI_LIBS MPILIBS)
    endif ()
    if (CGNS_ENABLE_PARALLEL)
      set(BUILDPARALLEL 1)
    endif ()
  endif ()

  set(cgns_C_FILES ${cgns_C_FILES} adfh/ADFH.c)
  if (CGNS_ENABLE_PARALLEL)
    set(cgns_C_FILES ${cgns_C_FILES} pcgnslib.c)
  endif ()

  # Check if HDF5 library has multi-dataset read/write APIs, HDF5 >= 1.14.0
  # --------------------------------------------------------------------

CHECK_HDF5_FEATURE(Multi_Dataset
  " 
 #include \"hdf5.h\"
 int main(void)
{
  H5Dwrite_multi(0, NULL, NULL, NULL, NULL, 0, NULL);
  return 0;
}
  "
   CHECK_HDF5_HAVE_MULTI_DATASETS
 )
 if (CHECK_HDF5_HAVE_MULTI_DATASETS)
   set(HDF5_HAVE_MULTI_DATASETS 1)
 endif ()

  # Check if HDF5 library has collective metadata APIs, HDF5 >= 1.10.0
  # --------------------------------------------------------------------

CHECK_HDF5_FEATURE(Collective_metadata
  " 
 #include \"hdf5.h\"
 int main(void)
 {
   hid_t foo;
   H5Pset_coll_metadata_write(foo, 1);
   H5Pset_all_coll_metadata_ops(foo, 1);
   return 0;
 }
  "
   CHECK_HDF5_HAVE_COLL_METADATA
 )
 if (CHECK_HDF5_HAVE_COLL_METADATA)
   set(HDF5_HAVE_COLL_METADATA 1)
 endif ()

 # Check if HDF5 library has H5Pset_file_space_strategy, HDF5 >= 1.10.1
 # --------------------------------------------------------------------

CHECK_HDF5_FEATURE(H5Pset_file_space_strategy
  " 
 #include \"hdf5.h\"
 int main(void)
 {
   hid_t foo;
   H5Pset_file_space_strategy(foo, H5F_FSPACE_STRATEGY_FSM_AGGR, 1, (hsize_t)1);
   return 0;
 }
  "
   CHECK_HDF5_HAVE_FILE_SPACE_STRATEGY
 )
 if (CHECK_HDF5_HAVE_FILE_SPACE_STRATEGY)
   set(HDF5_HAVE_FILE_SPACE_STRATEGY 1)
 endif ()

endif ()

  # Check for stat64 struct
  ## --------------------------------------------------------------------
  #  stat64 is deprecated for OS X

if (NOT APPLE)
  include(CheckStructHasMember)
  if ( WIN32 )
    CHECK_STRUCT_HAS_MEMBER("struct _stat64" st_atime sys/stat.h  CHECK_HAVE_STAT64_STRUCT LANGUAGE C)
  else ()
    CHECK_STRUCT_HAS_MEMBER("struct stat64" st_atime sys/stat.h  CHECK_HAVE_STAT64_STRUCT LANGUAGE C)
  endif()

  if (CHECK_HAVE_STAT64_STRUCT)
    set(HAVE_STAT64_STRUCT 1)
  endif ()
endif()

#create these in build directory so it doesn't mess up the
#source directory, then add the path to them
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnstypes.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnstypes.h )
if (FALSE) # XXX(kitware): No fortran support.
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnstypes_f.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnstypes_f.h )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnstypes_f03.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnstypes_f03.h )
endif ()
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnsconfig.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnsconfig.h )
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cg_hash_types.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cg_hash_types.h )
if (FALSE) # XXX(kitware): No need for this.
# The following is only useful with Unix Makefile Generator
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnsBuild.defs.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnsBuild.defs )
endif ()

###########
# Library #
###########

# All the C files needed by the cgns library
set(cgns_C_FILES
	cgns_error.c
	cgns_internals.c
	cgns_io.c
	cgnslib.c
	cg_hashmap.c
	adf/ADF_interface.c
	adf/ADF_internals.c)

if (CGNS_ENABLE_MEM_DEBUG)
  set(cgns_C_FILES ${cgns_C_FILES} cg_malloc.c)
endif()

if (CGNS_ENABLE_HDF5)
  set(cgns_C_FILES ${cgns_C_FILES} adfh/ADFH.c)
  if (CGNS_ENABLE_PARALLEL)
    set(cgns_C_FILES ${cgns_C_FILES} pcgnslib.c)
  endif ()
endif ()

# All the Fortran files need by the cgns library (if enabled)
set(cgns_F_FILES
	cg_ftoc.c
	cgio_ftoc.c)

# Only build those files that are needed
if (CGNS_ENABLE_FORTRAN)
  if (FORTRAN_NAMING)
    set_property(SOURCE ${cgns_F_FILES}
      PROPERTY COMPILE_DEFINITIONS ${FORTRAN_NAMING})
  endif ()
  set(cgns_FILES ${cgns_C_FILES} ${cgns_F_FILES})
  add_library(cgns_f2c OBJECT cgns_f.F90)
  # shared libraries need PIC
  set_property(TARGET cgns_f2c PROPERTY POSITION_INDEPENDENT_CODE 1)

  target_include_directories(cgns_f2c BEFORE PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
else ()
  set(cgns_FILES ${cgns_C_FILES})
endif ()

if (FALSE) # XXX(kitware): Handled by ParaView.
option(CGNS_USE_SHARED "Link programs to the CGNS shared library" "ON")

# Build a static version of the library
add_library(cgns_static STATIC ${cgns_FILES} $<$<BOOL:${CGNS_ENABLE_FORTRAN}>:$<TARGET_OBJECTS:cgns_f2c>>)
add_library(CGNS::cgns-static ALIAS cgns_static)
# Needed to work around a CMake > 3.8 bug on Windows with MSVS and Intel Fortran
set_property(TARGET cgns_static PROPERTY LINKER_LANGUAGE C)
target_link_libraries(cgns_static PRIVATE $<$<BOOL:${CGNS_ENABLE_HDF5}>:hdf5-${CG_HDF5_LINK_TYPE}>)

# Build a shared version of the library
if(CGNS_BUILD_SHARED)
  mark_as_advanced(CLEAR CGNS_USE_SHARED)
  add_library(cgns_shared SHARED ${cgns_FILES} $<$<BOOL:${CGNS_ENABLE_FORTRAN}>:$<TARGET_OBJECTS:cgns_f2c>>)
  add_library(CGNS::cgns-shared ALIAS cgns_shared)

  if (MSVC AND CGNS_ENABLE_FORTRAN)
    target_compile_definitions(cgns_f2c PRIVATE BUILD_CGNS_DLL)
  endif()

  if (WIN32 OR CYGWIN)
    target_compile_definitions(cgns_shared PRIVATE -DBUILD_DLL)
    target_compile_definitions(cgns_shared INTERFACE -DUSE_DLL)
  endif ()
  if (CGNS_ENABLE_HDF5 AND HDF5_LIBRARY)
    target_link_libraries(cgns_shared PUBLIC hdf5-${CG_HDF5_LINK_TYPE} $<$<NOT:$<PLATFORM_ID:Windows>>:${CMAKE_DL_LIBS}>)
    if(HDF5_NEED_ZLIB AND ZLIB_LIBRARY)
      target_link_libraries(cgns_shared PUBLIC ${ZLIB_LIBRARY})
    endif()
    if(HDF5_NEED_SZIP AND SZIP_LIBRARY)
      target_link_libraries(cgns_shared PUBLIC ${SZIP_LIBRARY})
    endif()
    if(HDF5_NEED_MPI AND MPI_LIBS)
      target_link_libraries(cgns_shared PUBLIC ${MPI_LIBS})
    endif()
  endif ()
else()
  set(CGNS_USE_SHARED "OFF")
  mark_as_advanced(FORCE CGNS_USE_SHARED)
endif()

# Include the local directory
target_include_directories(cgns_static BEFORE
	PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}

	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>)
if (CGNS_ENABLE_HDF5)
  if(HDF5_NEED_MPI AND MPI_INC)
    target_include_directories(cgns_static PRIVATE ${MPI_INC})
  endif()
endif ()

if (CGNS_BUILD_SHARED)
  target_include_directories(cgns_shared BEFORE
        PRIVATE
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}

	INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>)
  if (CGNS_ENABLE_HDF5)
    if(HDF5_NEED_MPI AND MPI_INC)
      target_include_directories(cgns_shared PRIVATE ${MPI_INC})
    endif()
  endif ()
endif()

if (CGNS_ENABLE_MEM_DEBUG)
  target_compile_definitions(cgns_static PRIVATE "-DMEM_DEBUG")
  if (CGNS_BUILD_SHARED)
    target_compile_definitions(cgns_shared PRIVATE "-DMEM_DEBUG")
  endif()
endif()


# Change the output name of the library to be libcgns
set_target_properties(cgns_static PROPERTIES OUTPUT_NAME cgns)
set_target_properties(cgns_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
if(CGNS_BUILD_SHARED)
  # for windows we need to change the name of the shared library
  # for both static and shared version to coexist
  if (CMAKE_STATIC_LIBRARY_SUFFIX STREQUAL CMAKE_IMPORT_LIBRARY_SUFFIX)
    set_target_properties(cgns_shared PROPERTIES OUTPUT_NAME cgnsdll)
  else ()
    set_target_properties(cgns_shared PROPERTIES OUTPUT_NAME cgns)
  endif ()
  set_target_properties(cgns_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
endif()

# Set the version numbers
set_target_properties(cgns_static PROPERTIES VERSION "${CGNS_VERSION}")
set_target_properties(cgns_static PROPERTIES SOVERSION "${CGNS_VERSION}")

if(CGNS_BUILD_SHARED)
  set_target_properties(cgns_shared PROPERTIES VERSION "${CGNS_VERSION}")
  set_target_properties(cgns_shared PROPERTIES SOVERSION "${CGNS_VERSION}")
endif()


set (install_targets cgns_static)
if(CGNS_BUILD_SHARED)
  set(install_targets ${install_targets} cgns_shared)
endif ()
# Set the install path of the static and shared library
# for windows, need to install both cgnsdll.dll and cgnsdll.lib
install (TARGETS ${install_targets}
         EXPORT cgns-targets
         LIBRARY DESTINATION lib COMPONENT libraries
         ARCHIVE DESTINATION lib COMPONENT libraries
         RUNTIME DESTINATION bin COMPONENT libraries
         INCLUDES DESTINATION include)

else ()
vtk_module_add_module(VTK::cgns
  SOURCES ${cgns_FILES})
set_target_properties(cgns PROPERTIES DEFINE_SYMBOL BUILD_DLL)
endif ()
# Set the install path of the header files
set(headers
  vtk_cgns_mangle.h
  cgnslib.h
  cgns_io.h
  #cgnswin_f.h
  ${CMAKE_CURRENT_BINARY_DIR}/cgnsconfig.h
  ${CMAKE_CURRENT_BINARY_DIR}/cgnstypes.h)
  #${CMAKE_CURRENT_BINARY_DIR}/cgnstypes_f.h
  #${CMAKE_CURRENT_BINARY_DIR}/cgnstypes_f03.h
  #${CMAKE_CURRENT_BINARY_DIR}/cgnsBuild.defs)

if (CGNS_ENABLE_FORTRAN)
  if(DEFINED CMAKE_Fortran_MODULE_DIRECTORY)
    list(APPEND headers
      ${CMAKE_Fortran_MODULE_DIRECTORY}/cgns.mod)
  else()
    list(APPEND headers
      $<TARGET_FILE_DIR:cgns_static>/cgns.mod)
  endif()
endif ()

if (CGNS_ENABLE_PARALLEL)
  list(APPEND headers
    pcgnslib.h)
endif ()

if (CGNS_ENABLE_LEGACY)
  list(APPEND headers
    adf/ADF.h)
  if (CGNS_ENABLE_HDF5)
    list(APPEND headers
      adfh/ADFH.h)
  endif ()
endif ()

if (FALSE) # XXX(kitware): Use the VTK module system.
# Set the install path of the header files
install(FILES ${headers}
	DESTINATION include)

#if (NOT CGNS_EXTERNALLY_CONFIGURE)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/cgns-config-version.cmake
  VERSION ${CGNS_VERSION}
  COMPATIBILITY SameMajorVersion )
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cgns-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cgns-config.cmake
  INSTALL_DESTINATION lib/cmake/cgns )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cgns-config-version.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/cgns-config.cmake
	DESTINATION lib/cmake/cgns)
install(EXPORT cgns-targets
	FILE cgns-targets.cmake
	NAMESPACE CGNS::
	DESTINATION lib/cmake/cgns
	)
#endif()
else ()
vtk_module_install_headers(
  FILES   ${headers}
  SUBDIR  "vtkcgns/src")
endif ()


#########
# Tools #
#########

if (FALSE) # XXX(kitware): VTK doesn't need this.
add_subdirectory(tools)

#########
# Tests #
#########

if (CGNS_ENABLE_TESTS)
  add_subdirectory(tests)
  add_subdirectory(Test_UserGuideCode/C_code)
  if (CGNS_ENABLE_FORTRAN)
    add_subdirectory(Test_UserGuideCode/Fortran_code)
  endif ()
  if (CGNS_ENABLE_PARALLEL)
    add_subdirectory(ptests)
    add_subdirectory(Test_UserGuideCode/C_code_parallel)
  endif ()
endif ()

##############
# CGNS Tools #
##############


set(CGNS_BUILD_CGNSTOOLS "OFF" CACHE BOOL "Build the CGNSTools package")
if(CGNS_BUILD_CGNSTOOLS)
  add_subdirectory(cgnstools)
endif()
endif ()
