unset(__priv_deps)
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  list(APPEND __priv_deps vtkglew)
endif()
vtk_module(vtkRenderingParallel
  DEPENDS
    vtkParallelCore
    vtkFiltersParallel
    vtkRendering${VTK_RENDERING_BACKEND}
  PRIVATE_DEPENDS
    vtkIOImage
    ${__priv_deps}
  TEST_DEPENDS
    vtkParallelMPI
    vtkFiltersParallelMPI
    vtkTestingRendering
    vtkImagingSources
    vtkRendering${VTK_RENDERING_BACKEND}
    vtkInteractionStyle
    vtkTestingCore
  KIT
    vtkParallel
  )
