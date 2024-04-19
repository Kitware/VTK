set(classes
  vtkDataEncoder
  vtkObjectIdMap
  vtkRemoteInteractionAdapter
  vtkWebApplication
  vtkWebInteractionEvent
  vtkWebUtilities)

vtk_module_add_module(VTK::WebCore
  CLASSES ${classes})
vtk_add_test_mangling(VTK::WebCore)
