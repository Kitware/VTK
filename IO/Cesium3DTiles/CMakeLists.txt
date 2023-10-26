set(classes
  vtkCesium3DTilesWriter
  vtkCesium3DTilesReader
  vtkCesiumPointCloudWriter)

set(private_classes
  TreeInformation)

vtk_module_add_module(VTK::IOCesium3DTiles
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  )
vtk_add_test_mangling(VTK::IOCesium3DTiles)
