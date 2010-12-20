cmake_minimum_required (VERSION 2.8)
PROJECT (HDF5_CPP)

#-----------------------------------------------------------------------------
# Shared/Static Libs
#-----------------------------------------------------------------------------
IF (BUILD_SHARED_LIBS)
  SET (CPP_BUILT_AS_DYNAMIC_LIB 1)
ENDIF (BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# Generate configure file
#-----------------------------------------------------------------------------
CONFIGURE_FILE (${HDF5_SOURCE_DIR}/Resources/H5cxx_config.h.in
                ${HDF5_BINARY_DIR}/H5cxx_pubconf.h
)

#-----------------------------------------------------------------------------
# Setup Include directories
#-----------------------------------------------------------------------------
INCLUDE_DIRECTORIES (${HDF5_CPP_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES (${HDF5_BINARY_DIR})

#-----------------------------------------------------------------------------
# Parallel/MPI, prevent spurious cpp/cxx warnings
#-----------------------------------------------------------------------------
IF (H5_HAVE_PARALLEL)
  ADD_DEFINITIONS ("-DMPICH_SKIP_MPICXX")
  ADD_DEFINITIONS ("-DMPICH_IGNORE_CXX_SEEK")
ENDIF (H5_HAVE_PARALLEL)

ADD_SUBDIRECTORY (${HDF5_CPP_SOURCE_DIR}/src ${HDF5_CPP_BINARY_DIR}/src)

#-----------------------------------------------------------------------------
# Build the CPP Examples
#-----------------------------------------------------------------------------
IF (HDF5_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY (${HDF5_CPP_SOURCE_DIR}/examples ${HDF5_CPP_BINARY_DIR}/examples)
ENDIF (HDF5_BUILD_EXAMPLES)

#-----------------------------------------------------------------------------
# Build the CPP unit tests
#-----------------------------------------------------------------------------
IF (BUILD_TESTING)
  ADD_SUBDIRECTORY (${HDF5_CPP_SOURCE_DIR}/test ${HDF5_CPP_BINARY_DIR}/test)
ENDIF (BUILD_TESTING)
