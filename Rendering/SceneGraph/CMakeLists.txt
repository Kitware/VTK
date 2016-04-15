set(Module_SRCS
  vtkActorNode.cxx
  vtkVolumeNode.cxx
  vtkCameraNode.cxx
  vtkLightNode.cxx
  vtkMapperNode.cxx
  vtkPolyDataMapperNode.cxx
  vtkVolumeMapperNode.cxx
  vtkRendererNode.cxx
  vtkViewNode.cxx
  vtkViewNodeCollection.cxx
  vtkViewNodeFactory.cxx
  vtkWindowNode.cxx
  )


set_source_files_properties(
  vtkViewNode
  ABSTRACT
  )

vtk_module_library(vtkRenderingSceneGraph ${Module_SRCS})
