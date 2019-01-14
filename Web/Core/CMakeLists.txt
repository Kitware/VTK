set(classes
  vtkDataEncoder
  vtkObjectIdMap
  vtkWebApplication
  vtkWebInteractionEvent
  vtkWebUtilities)

vtk_module_add_module(VTK::WebCore
  CLASSES ${classes})
