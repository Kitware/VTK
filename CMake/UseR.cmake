
# This module is used for compiling with Gnu R libraries and header files
#
# Defines the following:
#
# R_LIBRARIES - Libraries required to link Gnu R code

INCLUDE_DIRECTORIES(${R_INCLUDE_DIR})
LINK_DIRECTORIES(${R_LIB_DIR})

SET(R_LIBRARIES R)

