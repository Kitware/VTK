set(classes
  vtkActorNode
  vtkCameraNode
  vtkLightNode
  vtkMapperNode
  vtkPolyDataMapperNode
  vtkRendererNode
  vtkViewNode
  vtkViewNodeCollection
  vtkViewNodeFactory
  vtkVolumeMapperNode
  vtkVolumeNode
  vtkWindowNode)

vtk_module_add_module(VTK::RenderingSceneGraph
  CLASSES ${classes})
