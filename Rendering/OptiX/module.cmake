set(optional_test_depends)
if("${Module_vtkRenderingParallel}" AND "${Module_vtkParallelMPI}")
  set(optional_test_depends "vtkRenderingParallel;vtkParallelMPI")
endif()
vtk_module(vtkRenderingOptiX
  DEPENDS
    vtkRenderingSceneGraph
    vtkRenderingOpenGL2
  PRIVATE_DEPENDS
    vtkCommonSystem
  TEST_DEPENDS
    vtkFiltersTexture
    vtkInteractionStyle
    vtkIOGeometry
    vtkIOPLY
    vtkIOXML
    vtkRenderingAnnotation
    vtkTestingCore
    vtkTestingRendering
    ${optional_test_depends}
  EXCLUDE_FROM_ALL
  )
