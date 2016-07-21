# This extends CMake's FindHDF5.cmake to add support to include MPI include
# paths and libraries in the HDF5 ones if HDF5_IS_PARALLEL is ON
# (BUG #0014363).

# include the default FindHDF5.cmake.
if(CMAKE_VERSION VERSION_LESS 3.6.1)
  include(${CMAKE_CURRENT_LIST_DIR}/NewCMake/FindHDF5.cmake)
else()
  include(${CMAKE_ROOT}/Modules/FindHDF5.cmake)
endif()

if(HDF5_FOUND AND (HDF5_IS_PARALLEL OR HDF5_ENABLE_PARALLEL))
  include(vtkMPI)
  if(MPI_C_INCLUDE_PATH)
    list(APPEND HDF5_INCLUDE_DIRS ${MPI_C_INCLUDE_PATH})
  endif()
  if(MPI_C_LIBRARIES)
    list(APPEND HDF5_LIBRARIES ${MPI_C_LIBRARIES})
  endif()
  if(MPI_CXX_LIBRARIES)
    list(APPEND HDF5_LIBRARIES ${MPI_CXX_LIBRARIES})
  endif()
endif()
