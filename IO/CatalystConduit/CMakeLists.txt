set(classes
  vtkConduitArrayUtilities
  vtkConduitSource
  vtkDataObjectToConduit)

vtk_module_find_package(PACKAGE catalyst)

vtk_module_add_module(VTK::IOCatalystConduit
  CLASSES ${classes})

vtk_module_link(VTK::IOCatalystConduit
  PUBLIC
    catalyst::catalyst)
