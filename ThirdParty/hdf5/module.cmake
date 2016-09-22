if(BUILD_SHARED_LIBS)
  set(HDF5_USE_STATIC_LIBS FALSE)
else()
  set(HDF5_USE_STATIC_LIBS ON)
endif()
if(VTK_USE_SYSTEM_HDF5)
  set(vtkhdf5_LIBRARIES ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
  if(NOT HDF5_IS_PARALLEL)
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_INCLUDES ${HDF5_INCLUDE_DIRS})
    check_symbol_exists(H5_HAVE_PARALLEL hdf5.h HDF5_IS_PARALLEL)
    unset(CMAKE_REQUIRED_INCLUDES)
  endif()
  if (HDF5_IS_PARALLEL)
    include(vtkMPI)
    list(APPEND vtkhdf5_LIBRARIES
      ${MPI_C_LIBRARIES} ${MPI_CXX_LIBRARIES})
  endif ()
endif()
vtk_module(vtkhdf5
  EXCLUDE_FROM_WRAPPING
  DEPENDS
    vtkzlib
  )
