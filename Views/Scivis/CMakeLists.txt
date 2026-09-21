set(classes
  vtkBlockProperties
  vtkLookupTableManager
  vtkScivisDataRepresentation
  vtkScivisRepresentation
  vtkScivisScalarBars
  vtkScivisView
  vtkScivisSelector
  vtkSurfaceRepresentation
  vtkTextOverlayRepresentation
  vtkVolumeRepresentation
  )

vtk_module_add_module(VTK::ViewsScivis
  CLASSES ${classes})
