#
# This file provides functions for Fortran support.
#
#-------------------------------------------------------------------------------
ENABLE_LANGUAGE (Fortran)
  
#-----------------------------------------------------------------------------
# Detect name mangling convention used between Fortran and C
#-----------------------------------------------------------------------------
include (FortranCInterface)
FortranCInterface_HEADER (
    ${CMAKE_BINARY_DIR}/FCMangle.h
    MACRO_NAMESPACE "H5_FC_"
    SYMBOL_NAMESPACE "H5_FC_"
    SYMBOLS mysub mymod:my_sub
)

file (STRINGS ${CMAKE_BINARY_DIR}/FCMangle.h CONTENTS REGEX "H5_FC_GLOBAL\\(.*,.*\\) +(.*)")
string (REGEX MATCH "H5_FC_GLOBAL\\(.*,.*\\) +(.*)" RESULT ${CONTENTS})
set (H5_FC_FUNC "H5_FC_FUNC(name,NAME) ${CMAKE_MATCH_1}")

file (STRINGS ${CMAKE_BINARY_DIR}/FCMangle.h CONTENTS REGEX "H5_FC_GLOBAL_\\(.*,.*\\) +(.*)")
string (REGEX MATCH "H5_FC_GLOBAL_\\(.*,.*\\) +(.*)" RESULT ${CONTENTS})
set (H5_FC_FUNC_ "H5_FC_FUNC_(name,NAME) ${CMAKE_MATCH_1}")

#-----------------------------------------------------------------------------
# The provided CMake Fortran macros don't provide a general check function
# so this one is used for a sizeof test.
#-----------------------------------------------------------------------------
MACRO (CHECK_FORTRAN_FEATURE FUNCTION CODE VARIABLE)
  if (NOT DEFINED ${VARIABLE})
    message (STATUS "Testing Fortran ${FUNCTION}")
    if (CMAKE_REQUIRED_LIBRARIES)
      set (CHECK_FUNCTION_EXISTS_ADD_LIBRARIES
          "-DLINK_LIBRARIES:STRING=${CMAKE_REQUIRED_LIBRARIES}")
    else (CMAKE_REQUIRED_LIBRARIES)
      set (CHECK_FUNCTION_EXISTS_ADD_LIBRARIES)
    endif (CMAKE_REQUIRED_LIBRARIES)
    file (WRITE
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.f
        "${CODE}"
    )
    TRY_COMPILE (${VARIABLE}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/testFortranCompiler.f
        CMAKE_FLAGS "${CHECK_FUNCTION_EXISTS_ADD_LIBRARIES}"
        OUTPUT_VARIABLE OUTPUT
    )

#    message ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")
#    message ( "Test result ${OUTPUT}")
#    message ( "* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * ")

    if (${VARIABLE})
      set (${VARIABLE} 1 CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      message (STATUS "Testing Fortran ${FUNCTION} - OK")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
          "Determining if the Fortran ${FUNCTION} exists passed with the following output:\n"
          "${OUTPUT}\n\n"
      )
    else (${VARIABLE})
      message (STATUS "Testing Fortran ${FUNCTION} - Fail")
      set (${VARIABLE} "" CACHE INTERNAL "Have Fortran function ${FUNCTION}")
      file (APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
          "Determining if the Fortran ${FUNCTION} exists failed with the following output:\n"
          "${OUTPUT}\n\n")
    endif (${VARIABLE})
  endif (NOT DEFINED ${VARIABLE})
ENDMACRO (CHECK_FORTRAN_FEATURE)

#-----------------------------------------------------------------------------
# Configure Checks which require Fortran compilation must go in here
# not in the main ConfigureChecks.cmake files, because if the user has
# no Fortran compiler, problems arise.
#
# Be careful with leading spaces here, do not remove them.
#-----------------------------------------------------------------------------
CHECK_FORTRAN_FEATURE(sizeof
  "
       PROGRAM main
       i = sizeof(x)
       END PROGRAM
  "
  FORTRAN_HAVE_SIZEOF
)

CHECK_FORTRAN_FEATURE(RealIsNotDouble
  "
       MODULE type_mod
         INTERFACE h5t
           MODULE PROCEDURE h5t_real
           MODULE PROCEDURE h5t_dble
         END INTERFACE
       CONTAINS
         SUBROUTINE h5t_real(r)
           REAL :: r
         END SUBROUTINE h5t_real
         SUBROUTINE h5t_dble(d)
           DOUBLE PRECISION :: d
         END SUBROUTINE h5t_dble
       END MODULE type_mod
       PROGRAM main
         USE type_mod
         REAL :: r
         DOUBLE PRECISION :: d
         CALL h5t(r)
         CALL h5t(d)
       END PROGRAM main
  "
  FORTRAN_DEFAULT_REAL_NOT_DOUBLE
)

#-----------------------------------------------------------------------------
# Checks if the ISO_C_BINDING module meets all the requirements
#-----------------------------------------------------------------------------
CHECK_FORTRAN_FEATURE(iso_c_binding
  "
       PROGRAM main
            USE iso_c_binding
            IMPLICIT NONE
            TYPE(C_PTR) :: ptr
            TYPE(C_FUNPTR) :: funptr
            INTEGER(C_INT64_T) :: c_int64_type 
            CHARACTER(LEN=80, KIND=c_char), TARGET :: ichr
            ptr = C_LOC(ichr(1:1))
       END PROGRAM
  "
  FORTRAN_HAVE_ISO_C_BINDING
)

#-----------------------------------------------------------------------------
# Add debug information (intel Fortran : JB)
#-----------------------------------------------------------------------------
if (CMAKE_Fortran_COMPILER MATCHES ifort)
    if (WIN32)
        set (CMAKE_Fortran_FLAGS_DEBUG "/debug:full /dbglibs " CACHE "flags" STRING FORCE)
        set (CMAKE_EXE_LINKER_FLAGS_DEBUG "/DEBUG" CACHE "flags" STRING FORCE)
    endif (WIN32)
endif (CMAKE_Fortran_COMPILER MATCHES ifort)
