vtk_module(vtkImagingCore
  GROUPS
    StandAlone
  DEPENDS
    vtkCommonSystem
    vtkCommonTransforms
    vtkCommonMath
    vtkCommonComputationalGeometry
    vtkCommonExecutionModel
    vtkImagingMath
  TEST_DEPENDS
    vtkFiltersModeling
    vtkFiltersGeneral
    vtkFiltersHybrid
    vtkRenderingCore
    vtkTestingRendering
    vtkInteractionStyle
    vtkImagingStencil # Move tests
    vtkImagingGeneral # Move tests
    vtkImagingStatistics # Move tests
    vtkRenderingImage # Move tests
  )
