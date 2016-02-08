vtk_module(vtkRenderingOspray
  DEPENDS
    vtkRenderingSceneGraph
  TEST_DEPENDS
    vtkImagingSources #for composite test
    vtkParallelCore #for composite test
    vtkParallelMPI  #for composite test
    vtkRenderingParallel  #for composite test
    vtkFiltersTexture
    vtkInteractionStyle
    vtkIOPLY
    vtkIOXML
    vtkTestingCore
    vtkTestingRendering
    vtkRendering${VTK_RENDERING_BACKEND}
  EXCLUDE_FROM_ALL
  )
