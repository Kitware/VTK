vtk_module(vtkImagingCore
  GROUPS
    StandAlone
  DEPENDS
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkCommonMath
    vtkCommonSystem
    vtkCommonTransforms
  TEST_DEPENDS
    vtkIOLegacy
    vtkFiltersModeling
    vtkFiltersGeneral
    vtkFiltersHybrid
    vtkRenderingOpenGL
    vtkTestingRendering
    vtkInteractionStyle
    vtkInteractionImage
    vtkImagingMath # Move tests
    vtkImagingStencil # Move tests
    vtkImagingGeneral # Move tests
    vtkImagingSources
    vtkImagingStatistics # Move tests
    vtkRenderingImage # Move tests
  )
