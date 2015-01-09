# - Try to find the Metis graph partitioning library
# Once done this will define
#
#  METIS_FOUND - System has metis
#  METIS_INCLUDE_DIR - The metis include directory
#  METIS_LIBRARIES - The libraries needed to use metis

FIND_PATH(METIS_INCLUDE_DIR NAMES metis.h)

FIND_LIBRARY(METIS_LIBRARIES NAMES metis)

INCLUDE(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(Metis DEFAULT_MSG METIS_LIBRARIES METIS_INCLUDE_DIR)

MARK_AS_ADVANCED(METIS_INCLUDE_DIR METIS_LIBRARIES)
