if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(_imp_backend OpenGL2)
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
