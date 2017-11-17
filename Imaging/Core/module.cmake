vtk_module(vtkImagingCore
  GROUPS
    StandAlone
  TEST_DEPENDS
    vtkIOLegacy
    vtkFiltersModeling
    vtkFiltersGeneral
    vtkFiltersHybrid
    vtkRenderingOpenGL2
    vtkTestingRendering
    vtkInteractionStyle
    vtkInteractionImage
    vtkImagingMath # Move tests
    vtkImagingStencil # Move tests
    vtkImagingGeneral # Move tests
    vtkImagingSources
    vtkImagingStatistics # Move tests
    vtkRenderingImage # Move tests
  KIT
    vtkImaging
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
  PRIVATE_DEPENDS
    vtkCommonMath
    vtkCommonTransforms
  )
