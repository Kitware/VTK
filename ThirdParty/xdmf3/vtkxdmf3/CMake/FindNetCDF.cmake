#
# Find NetCDF include directories and libraries
#
# NetCDF_FOUND - System has NetCDF
# NetCDF_INCLUDE_DIR - The NetCDF include directory
# NetCDF_LIBRARIES - The libraries needed to use NetCDF

FIND_PATH(NetCDF_INCLUDE_DIR netcdf.h)

FIND_LIBRARY(NetCDF_LIBRARIES
  NAMES netcdf
  ${NetCDF_PREFIX}
  ${NetCDF_PREFIX}/lib64
  ${NetCDF_PREFIX}/lib
  /usr/local/lib64
  /usr/lib64
  /usr/lib64/netcdf-3
  /usr/local/lib
  /usr/lib
  /usr/lib/netcdf-3
)

INCLUDE(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set EXODUS_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(NetCDF DEFAULT_MSG NetCDF_INCLUDE_DIR NetCDF_LIBRARIES)

MARK_AS_ADVANCED(NetCDF_INCLUDE_DIR NetCDF_LIBRARIES)
