vtk_module(vtkRenderingContext2D
  TCL_NAME vtkRenderingContextIID
  GROUPS
    Rendering
  DEPENDS
    vtkRenderingCore
  PRIVATE_DEPENDS
    vtkCommonDataModel
    vtkCommonMath
    vtkCommonTransforms
    vtkRenderingFreeType
  TEST_DEPENDS
    vtkRenderingContext${VTK_RENDERING_BACKEND}
  KIT
    vtkRendering
  )
