set(_imp_backend OpenGL2)
if(VTK_RENDERING_BACKEND STREQUAL "OpenGL")
  set(_imp_backend "")
endif()
vtk_module(vtkRenderingParallelLIC
  IMPLEMENTS
    vtkRenderingLIC${_imp_backend}
  KIT
    vtkParallel
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkParallelMPI
    vtkRendering${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkIOLegacy
    vtkParallelCore
    vtkRenderingCore
  )
