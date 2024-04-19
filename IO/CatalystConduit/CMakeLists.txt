set(classes
  vtkConduitArrayUtilities
  vtkConduitSource
  vtkConduitToDataObject
  vtkDataObjectToConduit)

vtk_module_add_module(VTK::IOCatalystConduit
  CLASSES ${classes})

vtk_add_test_mangling(VTK::IOCatalystConduit)
add_subdirectory(Catalyst)
