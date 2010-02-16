
#
# - This module locates an installed R distribution.  
#
# Defines the following:
#
# R_INCLUDE_DIR - Path to R include directory
# R_LIBRARY     - Path to R library 
# R_COMMAND     - Path to R command
#

SET(TEMP_CMAKE_FIND_APPBUNDLE ${CMAKE_FIND_APPBUNDLE})
SET(CMAKE_FIND_APPBUNDLE "NEVER")
FIND_PROGRAM(R_COMMAND R DOC "R executable.")
IF (R_COMMAND)
  EXECUTE_PROCESS(WORKING_DIRECTORY . COMMAND ${R_COMMAND} RHOME OUTPUT_VARIABLE R_BASE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
  SET(VTK_R_HOME ${R_BASE_DIR} CACHE PATH "R home directory obtained from R RHOME")
ENDIF (R_COMMAND)
SET(CMAKE_FIND_APPBUNDLE ${TEMP_CMAKE_FIND_APPBUNDLE})

FIND_PATH(R_INCLUDE_DIR R.h PATHS /usr/local/lib /usr/local/lib64 PATH_SUFFIXES R/include DOC "Path to file R.h")
FIND_LIBRARY(R_LIBRARY_BASE R PATHS ${R_BASE_DIR} PATH_SUFFIXES /lib DOC "R library (example libR.a, libR.dylib, etc.).")
FIND_LIBRARY(R_LIBRARY_BLAS Rblas PATHS ${R_BASE_DIR} PATH_SUFFIXES /lib DOC "Rblas library (example libRblas.a, libRblas.dylib, etc.).")
FIND_LIBRARY(R_LIBRARY_LAPACK Rlapack PATHS ${R_BASE_DIR} PATH_SUFFIXES /lib  DOC "Rlapack library (example libRlapack.a, libRlapack.dylib, etc.).")
FIND_LIBRARY(R_LIBRARY_READLINE readline DOC "(Optional) system readline library. Only required if the R libraries were build with readline support.")

SET(R_LIBRARIES ${R_LIBRARY_BASE} ${R_LIBRARY_BLAS} ${R_LIBRARY_LAPACK} ${R_LIBRARY_BASE})
IF (R_LIBRARY_READLINE)
  SET(R_LIBRARIES ${R_LIBRARIES} ${R_LIBRARY_READLINE})
ENDIF (R_LIBRARY_READLINE)

