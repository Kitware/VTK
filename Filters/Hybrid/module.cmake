vtk_module(vtkFiltersHybrid
  GROUPS
    StandAlone
  DEPENDS
    vtkImagingSources
    vtkFiltersGeneral
    vtkRenderingCore # For vtkCamera in vtkDepthSortPolyData
  TEST_DEPENDS
    vtkIOXML
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkImagingCore
    vtkImagingStencil
    vtkTestingRendering
    vtkInteractionStyle
    vtkIOLegacy
  )
