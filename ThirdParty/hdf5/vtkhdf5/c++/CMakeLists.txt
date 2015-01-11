cmake_minimum_required (VERSION 2.8.11)
PROJECT (HDF5_CPP)

#-----------------------------------------------------------------------------
# Apply Definitions to compiler in this directory and below
#-----------------------------------------------------------------------------
add_definitions (${HDF5_EXTRA_C_FLAGS})

#-----------------------------------------------------------------------------
# Shared/Static Libs
#-----------------------------------------------------------------------------
if (BUILD_SHARED_LIBS)
  set (CPP_BUILT_AS_DYNAMIC_LIB 1)
else (BUILD_SHARED_LIBS)
  set (CPP_BUILT_AS_STATIC_LIB 1)
endif (BUILD_SHARED_LIBS)

#-----------------------------------------------------------------------------
# Generate configure file
#-----------------------------------------------------------------------------
configure_file (${HDF5_RESOURCES_DIR}/H5cxx_config.h.in
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
if (H5_HAVE_PARALLEL)
  add_definitions ("-DMPICH_SKIP_MPICXX")
  add_definitions ("-DMPICH_IGNORE_CXX_SEEK")
endif (H5_HAVE_PARALLEL)

add_subdirectory (${HDF5_CPP_SOURCE_DIR}/src ${HDF5_CPP_BINARY_DIR}/src)

#-----------------------------------------------------------------------------
# Build the CPP Examples
#-----------------------------------------------------------------------------
if (HDF5_BUILD_EXAMPLES)
  add_subdirectory (${HDF5_CPP_SOURCE_DIR}/examples ${HDF5_CPP_BINARY_DIR}/examples)
endif (HDF5_BUILD_EXAMPLES)

#-----------------------------------------------------------------------------
# Build the CPP unit tests
#-----------------------------------------------------------------------------
if (BUILD_TESTING)
  add_subdirectory (${HDF5_CPP_SOURCE_DIR}/test ${HDF5_CPP_BINARY_DIR}/test)
endif (BUILD_TESTING)
