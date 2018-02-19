if (VTK_USE_SYSTEM_HDF5)
  set(vtkhdf5_LIBRARIES ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
endif ()
vtk_module(vtkhdf5
  EXCLUDE_FROM_WRAPPING
  DEPENDS
    vtkzlib
  )
