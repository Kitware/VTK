vtk_module(vtkImagingCore
  GROUPS
    StandAlone
  DEPENDS
    vtkCommonSystem
    vtkCommonTransforms
    vtkCommonMath
    vtkCommonExecutionModel
  TEST_DEPENDS
    vtkFiltersModeling
    vtkFiltersGeneral
    vtkFiltersHybrid
    vtkRenderingCore
    vtkTestingRendering
    vtkInteractionImage
    vtkImagingMath # Move tests
    vtkImagingStencil # Move tests
    vtkImagingGeneral # Move tests
    vtkImagingStatistics # Move tests
    vtkRenderingImage # Move tests
  )
