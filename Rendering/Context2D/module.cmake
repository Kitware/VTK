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
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkRenderingFreeType
  )
