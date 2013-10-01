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
    vtkRenderingOpenGL
    vtkRenderingFreeType
  )
