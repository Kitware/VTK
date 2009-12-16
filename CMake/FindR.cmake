
#
# - This module locates an installed R distribution.  
#
# Defines the following:
#
# R_INCLUDE_DIR - Path to R include directory
# R_LIBRARY     - Path to R library 
# R_COMMAND     - Path to R command
#

FIND_PATH(R_INCLUDE_DIR "R.h" DOC "Path to file R.h")

FIND_LIBRARY(R_LIBRARY R DOC "R library (example libR.a, libR.dylib, etc.).")

FIND_PROGRAM(R_COMMAND R DOC "R executable.")

