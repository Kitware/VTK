vtk_module(vtkCommonColor
  DEPENDS
    vtkCommonDataModel # For vtkColor
  TEST_DEPENDS
    vtkIOImage
    vtkCommonExecutionModel
    vtkRenderingOpenGL
    vtkTestingCore
    vtkTestingRendering
)
