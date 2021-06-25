
##########
# Global #
##########

if (WIN32)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif (WIN32)

# Include the local directory
include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
if (CGNS_ENABLE_HDF5)
  if (FALSE) # XXX(kitware): Handled by ParaView.
  if (WIN32)
    if (HDF5_LIBRARY)
     # file(STRINGS ${HDF5_LIBRARY} HDF5isDLL REGEX "__imp__H5close")
     # if (HDF5isDLL)
        # this is only needed when using hdf5 dll libs
        add_definitions(-DWIN32 -D_HDF5USEDLL_)
     # endif (HDF5isDLL)
    endif (HDF5_LIBRARY)
  endif (WIN32)
  if(HDF5_NEED_MPI AND MPI_INC)
    include_directories(${MPI_INC})
  endif(HDF5_NEED_MPI AND MPI_INC)
  endif ()
endif (CGNS_ENABLE_HDF5)

################################
# create configured header files #
################################

if ("${SIZEOF_LONG}" GREATER 4)
  set(CGLONGT long)
else ("${SIZEOF_LONG}" GREATER 4)
  set(CGLONGT "long long")
endif ("${SIZEOF_LONG}" GREATER 4)

set(BUILDLEGACY 0)
set(BUILD64BIT 0)
set(BUILDSCOPE 0)
set(BUILDFORTRAN 0)
set(BUILDBASESCOPE 0)

if (CGNS_ENABLE_LEGACY)
  set(BUILDLEGACY 1)
else (CGNS_ENABLE_LEGACY)
  if (CGNS_ENABLE_64BIT)
    set(BUILD64BIT 1)
  else (CGNS_ENABLE_64BIT)
  endif (CGNS_ENABLE_64BIT)
endif (CGNS_ENABLE_LEGACY)
if (CGNS_ENABLE_SCOPING)
  set(BUILDSCOPE 1)
endif (CGNS_ENABLE_SCOPING)
if (CGNS_ENABLE_FORTRAN)
  set(BUILDFORTRAN 1)
endif (CGNS_ENABLE_FORTRAN)
if (CGNS_ENABLE_BASE_SCOPE)
  set(BUILDBASESCOPE 1)
endif (CGNS_ENABLE_BASE_SCOPE)

if (FALSE) # XXX(kitware): VTK handles flags better.
set(SYSCFLAGS "")
set(CFGFLAGS "")
set(LIBS "")

file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/lib LIBDIR)
file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/include INCLUDEDIR)
file(TO_NATIVE_PATH ${CMAKE_C_COMPILER} CC)
if (WIN32)
  set(CGNSLIB cgns.lib)
else (WIN32)
  set(CGNSLIB libcgns.a)
  set(LIBS -lm)
endif (WIN32)
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

string(COMPARE EQUAL ${CMAKE_BUILD_TYPE} Debug IS_DEBUG)
if (IS_DEBUG)
  set(BUILDDEBUG 1)
  set(CFLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_DEBUG}")
  set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
else (IS_DEBUG)
  set(BUILDDEBUG 0)
  set(CFLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_RELEASE}")
  set(LDFLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
endif (IS_DEBUG)
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
IF (CMAKE_Fortran_COMPILER_ID MATCHES "PGI")
 SET(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -Mx,125,0x200")
ENDIF()

#----------- Determine if the Fortran compiler NAG ------------------------
# The NAG compiler needs an additional flag for Fortran calls to C APIs.
#--------------------------------------------------------------------------
IF (CMAKE_Fortran_COMPILER_ID MATCHES "NAG")
 SET(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -mismatch_all")
ENDIF()

ENABLE_LANGUAGE(Fortran)

#-----------------------------------------------------------------------------
# The provided CMake Fortran macros don't provide a general check function
# so use this one.
#-----------------------------------------------------------------------------
MACRO (CHECK_FORTRAN_FEATURE FUNCTION CODE VARIABLE)
  IF (NOT DEFINED ${VARIABLE})
    IF (CMAKE_REQUIRED_LIBRARIES)
      SET (CHECK_FUNCTION_EXISTS_ADD_LIBRARIES
          "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    ELSE (CMAKE_REQUIRED_LIBRARIES)
      SET (CHECK_FUNCTION_EXISTS_ADD_LIBRARIES)
    ENDIF (CMAKE_REQUIRED_LIBRARIES)
    FILE (WRITE
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.F90
        "${CODE}"
    )
    TRY_COMPILE (${VARIABLE}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.F90
        CMAKE_FLAGS "${CHECK_FUNCTION_EXISTS_ADD_LIBRARIES}"
        OUTPUT_VARIABLE OUTPUT
    )

#    MESSAGE ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
#    MESSAGE ( "Test result ${OUTPUT}")
#    MESSAGE ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")

    IF (${VARIABLE})
      SET (${VARIABLE} 1 CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      MESSAGE (STATUS "Testing Fortran ${FUNCTION} - OK")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the Fortran ${FUNCTION} exists passed with the following output:\n"
          "${OUTPUT}\n\n"
      )
    ELSE (${VARIABLE})
      MESSAGE (STATUS "Testing Fortran ${FUNCTION} - Fail")
      SET (${VARIABLE} "" CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the Fortran ${FUNCTION} exists failed with the following output:\n"
          "${OUTPUT}\n\n")
    ENDIF (${VARIABLE})
  ENDIF (NOT DEFINED ${VARIABLE})
ENDMACRO (CHECK_FORTRAN_FEATURE)

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
endif (CHECK_FORTRAN_2003)
   

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
endif (CHECK_FORTRAN_2008)

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
endif (NOT CHECK_FORTRAN_DEFAULT_INTEGER_C_INT32_T)

else (HAVE_FORTRAN_2003)

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
endif (CHECK_FORTRAN_95)
endif (HAVE_FORTRAN_2003)

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
endif (NOT CHECK_FORTRAN_CONCAT)

endif (BUILDFORTRAN)

macro(native_paths INPUT_PATH_VARIABLE RESULT_VARIABLE)
  set(${RESULT_VARIABLE} "")
  if (FALSE) # XXX(kitware): This hard-codes full paths into the build; removing.
  foreach(PATH ${${INPUT_PATH_VARIABLE}})
    file(TO_NATIVE_PATH ${PATH} NATIVE_PATH)
    list(APPEND ${RESULT_VARIABLE} ${NATIVE_PATH})
  endforeach(PATH ${${INPUT_PATH_VARIABLE}})
  endif ()
endmacro(native_paths)

#-----------------------------------------------------------------------------
# The provided CMake macros don't provide a general HDF5 check function
# so use this one.
#-----------------------------------------------------------------------------

MACRO (CHECK_HDF5_FEATURE FUNCTION CODE VARIABLE)
  IF (NOT DEFINED ${VARIABLE})
    FILE (WRITE
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_HDF.c
        "${CODE}"
    )
SET (CHECK_FUNCTION_EXISTS_ADD_INCLUDE "-DINCLUDE_DIRECTORIES:STRING=${HDF5_INCLUDE_DIR}")
    TRY_COMPILE (${VARIABLE}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/test_HDF.c
        LINK_LIBRARIES ${HDF5_LIBRARY}
        CMAKE_FLAGS "${CHECK_FUNCTION_EXISTS_ADD_INCLUDE}"
        OUTPUT_VARIABLE OUTPUT
    )

#    MESSAGE ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
#    MESSAGE ( "Test result ${OUTPUT}")
#    MESSAGE ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")

    IF (${VARIABLE})
      SET (${VARIABLE} 1 CACHE INTERNAL "Have HDF5 function ${FUNCTION}")
      MESSAGE (STATUS "Testing HDF5 ${FUNCTION} - OK")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the HDF5 ${FUNCTION} exists passed with the following output:\n"
          "${OUTPUT}\n\n"
      )
    ELSE (${VARIABLE})
      MESSAGE (STATUS "Testing HDF5 ${FUNCTION} - NOT FOUND")
      SET (${VARIABLE} "" CACHE INTERNAL "Have HDF5 function ${FUNCTION}")
      FILE (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the HDF5 ${FUNCTION} exists failed with the following output:\n"
          "${OUTPUT}\n\n")
    ENDIF (${VARIABLE})
  ENDIF (NOT DEFINED ${VARIABLE})
ENDMACRO (CHECK_HDF5_FEATURE)

if (CGNS_ENABLE_HDF5)
  set(BUILDHDF5 1)
  if (HDF5_INCLUDE_PATH)
    native_paths(HDF5_INCLUDE_PATH HDF5INC)
	include_directories(${HDF5INC})
  endif (HDF5_INCLUDE_PATH)
  if (HDF5_LIBRARY)
    native_paths(HDF5_LIBRARY HDF5LIB)
  endif (HDF5_LIBRARY)
  if (HDF5_NEED_SZIP)
    set(H5NEEDSZIP 1)
    if (SZIP_LIBRARY)
      native_paths(SZIP_LIBRARY SZIPLIB)
    endif (SZIP_LIBRARY)
  endif (HDF5_NEED_SZIP)
  if (HDF5_NEED_ZLIB)
    set(H5NEEDZLIB 1)
    if (ZLIB_LIBRARY)
      native_paths(ZLIB_LIBRARY ZLIBLIB)
    endif (ZLIB_LIBRARY)
  endif (HDF5_NEED_ZLIB)
  if (HDF5_NEED_MPI)
    set(H5NEEDMPI 1)
    if (MPI_INC)
      native_paths(MPI_INC MPIINC)
    endif (MPI_INC)
    if (MPI_LIBS)
      native_paths(MPI_LIBS MPILIBS)
    endif (MPI_LIBS)
    if (CGNS_ENABLE_PARALLEL)
      set(BUILDPARALLEL 1)
    endif (CGNS_ENABLE_PARALLEL)
  endif (HDF5_NEED_MPI)

  set(cgns_C_FILES ${cgns_C_FILES} adfh/ADFH.c)
  if (CGNS_ENABLE_PARALLEL)
    set(cgns_C_FILES ${cgns_C_FILES} pcgnslib.c)
  endif (CGNS_ENABLE_PARALLEL)

  # Check if HDF5 library has multi-dataset read/write APIs
  # --------------------------------------------------------------------

CHECK_HDF5_FEATURE(Multi_Dataset
  " 
 #include \"hdf5.h\"
 void main(void) 
{
  H5D_rw_multi_t *multi_info;
  return;
}
  "
   CHECK_HDF5_HAVE_MULTI_DATASETS
 )
 if (CHECK_HDF5_HAVE_MULTI_DATASETS)
   set(HDF5_HAVE_MULTI_DATASETS 1)
 endif (CHECK_HDF5_HAVE_MULTI_DATASETS)

  # Check if HDF5 library has collective metadata APIs, HDF5 >= 1.10.0
  # --------------------------------------------------------------------

CHECK_HDF5_FEATURE(Collective_metadata
  " 
 #include \"hdf5.h\"
 void main(void)
 {
   hid_t foo;
   H5Pset_coll_metadata_write(foo, 1);
   H5Pset_all_coll_metadata_ops(foo, 1);
   return;
 }
  "
   CHECK_HDF5_HAVE_COLL_METADATA
 )
 if (CHECK_HDF5_HAVE_COLL_METADATA)
   set(HDF5_HAVE_COLL_METADATA 1)
 endif (CHECK_HDF5_HAVE_COLL_METADATA)

 # Check if HDF5 library has H5Pset_file_space_strategy, HDF5 >= 1.10.1
 # --------------------------------------------------------------------

CHECK_HDF5_FEATURE(H5Pset_file_space_strategy
  " 
 #include \"hdf5.h\"
 void main(void)
 {
   hid_t foo;
   H5Pset_file_space_strategy(foo, H5F_FSPACE_STRATEGY_FSM_AGGR, 1, (hsize_t)1);
   return;
 }
  "
   CHECK_HDF5_HAVE_FILE_SPACE_STRATEGY
 )
 if (CHECK_HDF5_HAVE_FILE_SPACE_STRATEGY)
   set(HDF5_HAVE_FILE_SPACE_STRATEGY 1)
 endif (CHECK_HDF5_HAVE_FILE_SPACE_STRATEGY)

endif (CGNS_ENABLE_HDF5)

  # Check for stat64 struct
  ## --------------------------------------------------------------------
  #  stat64 is deprecated for OS X

if (NOT APPLE)
  include(CheckStructHasMember)
  if ( WIN32 )
    CHECK_STRUCT_HAS_MEMBER("struct _stat64" st_atime sys/stat.h  CHECK_HAVE_STAT64_STRUCT LANGUAGE C)
  else ( WIN32 )
    CHECK_STRUCT_HAS_MEMBER("struct stat64" st_atime sys/stat.h  CHECK_HAVE_STAT64_STRUCT LANGUAGE C)
  endif( WIN32)

  if (CHECK_HAVE_STAT64_STRUCT)
    set(HAVE_STAT64_STRUCT 1)
  endif (CHECK_HAVE_STAT64_STRUCT)
endif(NOT APPLE)

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
if (FALSE) # XXX(kitware): No need for this.
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cgnsBuild.defs.in
		${CMAKE_CURRENT_BINARY_DIR}/cgnsBuild.defs )
endif ()
configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cg_hash_types.h.in
		${CMAKE_CURRENT_BINARY_DIR}/cg_hash_types.h )

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})

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
  add_definitions("-DMEM_DEBUG")
endif(CGNS_ENABLE_MEM_DEBUG)

if (CGNS_ENABLE_HDF5)
  set(cgns_C_FILES ${cgns_C_FILES} adfh/ADFH.c)
  if (CGNS_ENABLE_PARALLEL)
    set(cgns_C_FILES ${cgns_C_FILES} pcgnslib.c)
  endif (CGNS_ENABLE_PARALLEL)
endif (CGNS_ENABLE_HDF5)

# All the Fortran files need by the cgns library (if enabled)
set(cgns_F_FILES
	cg_ftoc.c
	cgio_ftoc.c)

# Only build those files that are needed
if (CGNS_ENABLE_FORTRAN)
  if (FORTRAN_NAMING)
    set_property(SOURCE ${cgns_F_FILES}
      PROPERTY COMPILE_DEFINITIONS ${FORTRAN_NAMING})
  endif (FORTRAN_NAMING)
  set(cgns_FILES ${cgns_C_FILES} ${cgns_F_FILES})
  add_library(cgns_f2c OBJECT cgns_f.F90)
else (CGNS_ENABLE_FORTRAN)
  set(cgns_FILES ${cgns_C_FILES})
endif (CGNS_ENABLE_FORTRAN)

if (FALSE) # XXX(kitware): Handled by ParaView.
option(CGNS_USE_SHARED "Link programs to the CGNS shared library" "ON")

# Build a static version of the library
if (CGNS_ENABLE_FORTRAN)
  add_library(cgns_static STATIC ${cgns_FILES} $<TARGET_OBJECTS:cgns_f2c>)
else (CGNS_ENABLE_FORTRAN)
  add_library(cgns_static STATIC ${cgns_FILES})
endif (CGNS_ENABLE_FORTRAN)
# Needed to work around a CMake > 3.8 bug on Windows with MSVS and Intel Fortran
set_property(TARGET cgns_static PROPERTY LINKER_LANGUAGE C)

# Build a shared version of the library
if(CGNS_BUILD_SHARED)
  mark_as_advanced(CLEAR CGNS_USE_SHARED)
  if (CGNS_ENABLE_FORTRAN)
    add_library(cgns_shared SHARED ${cgns_FILES} $<TARGET_OBJECTS:cgns_f2c>)
  else (CGNS_ENABLE_FORTRAN)
    add_library(cgns_shared SHARED ${cgns_FILES})
  endif (CGNS_ENABLE_FORTRAN)
  # under windows we need to define BUILD_DLL
  # and link with the HDF5 libraries to build a dll
  if (WIN32 OR CYGWIN)
    target_compile_definitions(cgns_shared PRIVATE -DBUILD_DLL)
    target_compile_definitions(cgns_shared INTERFACE -DUSE_DLL)
  endif (WIN32 OR CYGWIN)
  if (CGNS_ENABLE_HDF5 AND HDF5_LIBRARY)
    target_link_libraries(cgns_shared ${HDF5_LIBRARY} ${CMAKE_DL_LIBS})
    if(HDF5_NEED_ZLIB AND ZLIB_LIBRARY)
      target_link_libraries(cgns_shared ${ZLIB_LIBRARY})
    endif(HDF5_NEED_ZLIB AND ZLIB_LIBRARY)
    if(HDF5_NEED_SZIP AND SZIP_LIBRARY)
      target_link_libraries(cgns_shared ${SZIP_LIBRARY})
    endif(HDF5_NEED_SZIP AND SZIP_LIBRARY)
    if(HDF5_NEED_MPI AND MPI_LIBS)
      target_link_libraries(cgns_shared ${MPI_LIBS})
    endif(HDF5_NEED_MPI AND MPI_LIBS)
  endif (CGNS_ENABLE_HDF5 AND HDF5_LIBRARY)
else(CGNS_BUILD_SHARED)
  set(CGNS_USE_SHARED "OFF")
  mark_as_advanced(FORCE CGNS_USE_SHARED)
endif(CGNS_BUILD_SHARED)

# Change the output name of the library to be libcgns
set_target_properties(cgns_static PROPERTIES OUTPUT_NAME cgns)
set_target_properties(cgns_static PROPERTIES CLEAN_DIRECT_OUTPUT 1)
if(CGNS_BUILD_SHARED)
  # for windows we need to change the name of the shared library
  # for both static and shared version to exist
  if (WIN32 OR CYGWIN)
    set_target_properties(cgns_shared PROPERTIES OUTPUT_NAME cgnsdll)
  else (WIN32 OR CYGWIN)
    set_target_properties(cgns_shared PROPERTIES OUTPUT_NAME cgns)
  endif (WIN32 OR CYGWIN)
  set_target_properties(cgns_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
endif(CGNS_BUILD_SHARED)

# Set the version numbers
set_target_properties(cgns_static PROPERTIES VERSION "${CGNS_VERSION}")
set_target_properties(cgns_static PROPERTIES SOVERSION "${CGNS_VERSION}")

if(CGNS_BUILD_SHARED)
  set_target_properties(cgns_shared PROPERTIES VERSION "${CGNS_VERSION}")
  set_target_properties(cgns_shared PROPERTIES SOVERSION "${CGNS_VERSION}")
endif(CGNS_BUILD_SHARED)

# Set the install path of the static library
install(TARGETS cgns_static ARCHIVE DESTINATION lib)
# Set the install path of the shared library
if(CGNS_BUILD_SHARED)
  # for windows, need to install both cgnsdll.dll and cgnsdll.lib
  install (TARGETS cgns_shared LIBRARY DESTINATION lib
                               ARCHIVE DESTINATION lib
                               RUNTIME DESTINATION bin)
endif(CGNS_BUILD_SHARED)
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
endif (CGNS_ENABLE_FORTRAN)

if (CGNS_ENABLE_PARALLEL)
  list(APPEND headers
    pcgnslib.h)
endif (CGNS_ENABLE_PARALLEL)

if (CGNS_ENABLE_LEGACY)
  list(APPEND headers
    adf/ADF.h)
  if (CGNS_ENABLE_HDF5)
    list(APPEND headers
      adfh/ADFH.h)
  endif (CGNS_ENABLE_HDF5)
endif (CGNS_ENABLE_LEGACY)

if (FALSE) # XXX(kitware): Use the VTK module system.
# Set the install path of the header files
install(FILES ${headers}
	DESTINATION include)
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
  if (CGNS_ENABLE_PARALLEL)
    add_subdirectory(ptests)
    add_subdirectory(Test_UserGuideCode/C_code_parallel)
  endif (CGNS_ENABLE_PARALLEL)
endif (CGNS_ENABLE_TESTS)

##############
# CGNS Tools #
##############


set(CGNS_BUILD_CGNSTOOLS "OFF" CACHE BOOL "Build the CGNSTools package")
if(CGNS_BUILD_CGNSTOOLS)
  add_subdirectory(cgnstools)
endif(CGNS_BUILD_CGNSTOOLS)
endif ()
