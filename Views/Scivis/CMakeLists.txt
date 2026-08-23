set(classes
  vtkBlockProperties
  vtkScivisDataRepresentation
  vtkScivisRepresentation
  vtkScivisView
  vtkScivisSelector
  vtkSurfaceRepresentation
  vtkVolumeRepresentation
  )

vtk_module_add_module(VTK::ViewsScivis
  CLASSES ${classes})
