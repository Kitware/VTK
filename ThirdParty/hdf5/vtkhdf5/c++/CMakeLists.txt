<<<<<<< HEAD
||||||| merged common ancestors
cmake_minimum_required (VERSION 2.8)
=======
cmake_minimum_required (VERSION 2.8.11)
>>>>>>> hdf5-upstream
PROJECT (HDF5_CPP)

#-----------------------------------------------------------------------------
# Apply Definitions to compiler in this directory and below
#-----------------------------------------------------------------------------
add_definitions (${HDF5_EXTRA_C_FLAGS})

#-----------------------------------------------------------------------------
# Shared/Static Libs
#-----------------------------------------------------------------------------
<<<<<<< HEAD
IF (BUILD_SHARED_LIBS)
  SET (CPP_BUILT_AS_DYNAMIC_LIB 1)
ENDIF ()
||||||| merged common ancestors
IF (BUILD_SHARED_LIBS)
  SET (CPP_BUILT_AS_DYNAMIC_LIB 1)
ENDIF (BUILD_SHARED_LIBS)
=======
if (BUILD_SHARED_LIBS)
  set (CPP_BUILT_AS_DYNAMIC_LIB 1)
else (BUILD_SHARED_LIBS)
  set (CPP_BUILT_AS_STATIC_LIB 1)
endif (BUILD_SHARED_LIBS)
>>>>>>> hdf5-upstream

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
<<<<<<< HEAD
IF (H5_HAVE_PARALLEL)
  ADD_DEFINITIONS ("-DMPICH_SKIP_MPICXX")
  ADD_DEFINITIONS ("-DMPICH_IGNORE_CXX_SEEK")
ENDIF ()
||||||| merged common ancestors
IF (H5_HAVE_PARALLEL)
  ADD_DEFINITIONS ("-DMPICH_SKIP_MPICXX")
  ADD_DEFINITIONS ("-DMPICH_IGNORE_CXX_SEEK")
ENDIF (H5_HAVE_PARALLEL)
=======
if (H5_HAVE_PARALLEL)
  add_definitions ("-DMPICH_SKIP_MPICXX")
  add_definitions ("-DMPICH_IGNORE_CXX_SEEK")
endif (H5_HAVE_PARALLEL)
>>>>>>> hdf5-upstream

add_subdirectory (${HDF5_CPP_SOURCE_DIR}/src ${HDF5_CPP_BINARY_DIR}/src)

#-----------------------------------------------------------------------------
# Build the CPP Examples
#-----------------------------------------------------------------------------
<<<<<<< HEAD
IF (HDF5_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY (${HDF5_CPP_SOURCE_DIR}/examples ${HDF5_CPP_BINARY_DIR}/examples)
ENDIF ()
||||||| merged common ancestors
IF (HDF5_BUILD_EXAMPLES)
  ADD_SUBDIRECTORY (${HDF5_CPP_SOURCE_DIR}/examples ${HDF5_CPP_BINARY_DIR}/examples)
ENDIF (HDF5_BUILD_EXAMPLES)
=======
if (HDF5_BUILD_EXAMPLES)
  add_subdirectory (${HDF5_CPP_SOURCE_DIR}/examples ${HDF5_CPP_BINARY_DIR}/examples)
endif (HDF5_BUILD_EXAMPLES)
>>>>>>> hdf5-upstream

#-----------------------------------------------------------------------------
# Build the CPP unit tests
#-----------------------------------------------------------------------------
<<<<<<< HEAD
IF (BUILD_TESTING)
  ADD_SUBDIRECTORY (${HDF5_CPP_SOURCE_DIR}/test ${HDF5_CPP_BINARY_DIR}/test)
ENDIF ()
||||||| merged common ancestors
IF (BUILD_TESTING)
  ADD_SUBDIRECTORY (${HDF5_CPP_SOURCE_DIR}/test ${HDF5_CPP_BINARY_DIR}/test)
ENDIF (BUILD_TESTING)
=======
if (BUILD_TESTING)
  add_subdirectory (${HDF5_CPP_SOURCE_DIR}/test ${HDF5_CPP_BINARY_DIR}/test)
endif (BUILD_TESTING)
>>>>>>> hdf5-upstream
