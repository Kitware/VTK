vtk_module(vtkCommonColor
  DEPENDS
    vtkCommonDataModel # For vtkColor
  TEST_DEPENDS
    vtkIOImage
    vtkCommonExecutionModel
    vtkFiltersModeling # For vtkBandedPolyDataContourFilter used in a test
    vtkRenderingOpenGL
    vtkTestingCore
    vtkTestingRendering
)
