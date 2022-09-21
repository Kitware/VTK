set(classes
  vtkActorNode
  vtkCameraNode
  vtkLightNode
  vtkMapperNode
  vtkPolyDataMapperNode
  vtkRendererNode
  vtkViewNode
  vtkViewNodeFactory
  vtkVolumeMapperNode
  vtkVolumeNode
  vtkWindowNode)

vtk_module_add_module(VTK::RenderingSceneGraph
  CLASSES ${classes})
vtk_add_test_mangling(VTK::RenderingSceneGraph)
