vtk_module(vtkCommonColor
  DEPENDS
    vtkCommonDataModel # For vtkColor
  TEST_DEPENDS
    vtkIOImage
    vtkCommonExecutionModel
    vtkRenderingOpenGL2
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionStyle
)
