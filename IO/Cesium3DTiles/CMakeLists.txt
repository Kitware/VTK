set(classes
  vtkCesium3DTilesWriter
  vtkCesiumPointCloudWriter)

set(private_classes
  TreeInformation)

vtk_module_add_module(VTK::IOCesium3DTiles
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  )
