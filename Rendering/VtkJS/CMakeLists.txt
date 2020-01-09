set(classes
  vtkVtkJSSceneGraphSerializer
  vtkVtkJSViewNodeFactory
)

vtk_module_add_module(VTK::RenderingVtkJS
  CLASSES ${classes})
