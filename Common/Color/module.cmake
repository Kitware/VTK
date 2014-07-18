vtk_module(vtkCommonColor
  DEPENDS
    vtkCommonDataModel # For vtkColor
  TEST_DEPENDS
    vtkIOImage
    vtkCommonExecutionModel
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkTestingCore
    vtkTestingRendering
    vtkInteractionStyle
  KIT
    vtkCommon
)
