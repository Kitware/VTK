vtk_module(vtkTestingGenericBridge
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
  PRIVATE_DEPENDS
    vtkCommonMisc
  EXCLUDE_FROM_WRAPPING)