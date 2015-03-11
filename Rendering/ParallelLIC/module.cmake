if(VTK_RENDERING_BACKEND STREQUAL "OpenGL2")
  set(_imp_backend OpenGL2)
endif()

vtk_module(vtkRenderingParallelLIC
  IMPLEMENTS
    vtkRenderingLIC${_imp_backend}
  DEPENDS
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkParallelMPI
    vtkIOLegacy
  KIT
    vtkParallel
  )
