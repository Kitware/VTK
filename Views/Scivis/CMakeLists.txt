set(classes
  vtkBlockProperties
  vtkLookupTableManager
  vtkScivisDataRepresentation
  vtkScivisRepresentation
  vtkScivisView
  vtkScivisSelector
  vtkSurfaceRepresentation
  vtkVolumeRepresentation
  )

vtk_module_add_module(VTK::ViewsScivis
  CLASSES ${classes})
