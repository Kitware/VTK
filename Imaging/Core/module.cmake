vtk_module(vtkImagingCore
  GROUPS
    StandAlone
  DEPENDS
    vtkCommonSystem
    vtkCommonTransforms
    vtkCommonMath
    vtkCommonExecutionModel
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
    vtkImagingStatistics # Move tests
    vtkRenderingImage # Move tests
  )
