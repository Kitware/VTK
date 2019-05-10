set(classes
  vtkH5PartReader)

vtk_module_add_module(VTK::IOH5part
  CLASSES ${classes})

vtk_module_definitions(VTK::IOH5part
  PRIVATE
    H5_USE_16_API)
