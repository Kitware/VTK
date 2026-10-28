set(classes
  vtkBlockProperties
  vtkGridAxesRepresentation
  vtkLookupTableManager
  vtkScivisDataRepresentation
  vtkScivisExporter
  vtkScivisRepresentation
  vtkScivisScalarBars
  vtkScivisView
  vtkScivisWidgets
  vtkScivisSelector
  vtkSurfaceRepresentation
  vtkTextOverlayRepresentation
  vtkVolumeRepresentation
  )

vtk_module_add_module(VTK::ViewsScivis
  CLASSES ${classes})
