vtk_module(vtkhdf5
  DEPENDS
    vtkzlib
  EXCLUDE_FROM_WRAPPING
  )
if(VTK_USE_SYSTEM_HDF5)
  set(vtkhdf5_LIBRARIES ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES})
endif()
