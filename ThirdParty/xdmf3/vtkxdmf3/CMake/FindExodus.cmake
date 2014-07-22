#
# Find the Exodus finite element data model library from Sandia
#
#  EXODUS_FOUND - System has Exodus
#  EXODUS_INCLUDE_DIR - The LibXml2 include directory
#  EXODUS_LIBRARIES - The libraries needed to use LibXml2

FIND_PACKAGE(NetCDF REQUIRED)

FIND_PATH(EXODUS_INCLUDE_DIR NAMES exodusII.h)

FIND_LIBRARY(EXODUS_LIBRARIES NAMES exodusii exodusIIv2c)

INCLUDE(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set EXODUS_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Exodus DEFAULT_MSG EXODUS_LIBRARIES EXODUS_INCLUDE_DIR)

MARK_AS_ADVANCED(EXODUS_INCLUDE_DIR EXODUS_LIBRARIES)

