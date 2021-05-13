set(classes
  vtk3DTilesWriter)

set(private_classes
  TreeInformation)

vtk_module_add_module(VTK::IO3DTiles
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  )
