#
# To be used by projects that make use of Cmakeified hdf5-1.8
#

#
# Find the HDF5 includes and get all installed hdf5 library settings from
# HDF5-config.cmake file : Requires a CMake compatible hdf5-1.8.5 or later 
# for this feature to work. The following vars are set if hdf5 is found.
#
# HDF5_FOUND               - True if found, otherwise all other vars are undefined
# HDF5_INCLUDE_DIR         - The include dir for main *.h files
# HDF5_FORTRAN_INCLUDE_DIR - The include dir for fortran modules and headers
# HDF5_VERSION_STRING      - full version (e.g. 1.8.5)
# HDF5_VERSION_MAJOR       - major part of version (e.g. 1.8)
# HDF5_VERSION_MINOR       - minor part (e.g. 5)
# 
# The following boolean vars will be defined
# HDF5_ENABLE_PARALLEL - 1 if HDF5 parallel supported
# HDF5_BUILD_FORTRAN   - 1 if HDF5 was compiled with fortran on
# HDF5_BUILD_CPP_LIB   - 1 if HDF5 was compiled with cpp on
# HDF5_BUILD_TOOLS     - 1 if HDF5 was compiled with tools on
# HDF5_BUILD_HL_LIB    - 1 if HDF5 was compiled with parallel on
# 
# Target names that are valid (depending on enabled options)
# will be the following
#
# hdf5              : HDF5 C library
# hdf5_tools        : the tools library
# hdf5_f90cstub     : used by Fortran to C interface
# hdf5_fortran      : Fortran HDF5 library
# hdf5_cpp          : HDF5 cpp interface library
# hdf5_hl           : High Level library
# hdf5_hl_f90cstub  : used by Fortran to C interface to High Level library
# hdf5_hl_fortran   : Fortran High Level library
# hdf5_hl_cpp       : High Level cpp interface library
# 
# To aid in finding HDF5 as part of a subproject set
# HDF5_ROOT_DIR_HINT to the location where HDF5-config.cmake lies

FIND_PATH (HDF5_ROOT_DIR "HDF5-config.cmake"
    ${HDF5_ROOT_DIR_HINT}
    /usr/local/lib
    /usr/local/lib64
    /usr/lib
    /usr/lib64
    "C:/Program Files/HDF5/lib"
)

FIND_PATH (HDF5_INCLUDE_DIR "H5public.h"
    ${HDF5_ROOT_DIR}/../include
)

IF (HDF5_INCLUDE_DIR)
  SET (HDF5_FOUND "YES")
  INCLUDE (${HDF5_ROOT_DIR}/HDF5-config.cmake)
ENDIF (HDF5_INCLUDE_DIR)
