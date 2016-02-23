set(optional_test_depends)
if(${Module_vtkRenderingParallel} AND ${Module_vtkParallelMPI})
  set(optional_test_depends "vtkRenderingParallel;vtkParallelMPI")
endif()

vtk_module(vtkRenderingOspray
  DEPENDS
    vtkRenderingSceneGraph
    #todo promote compositedatadisplayattributes to rendering/core
    vtkRendering${VTK_RENDERING_BACKEND} #only for comp.data.disp.attr.
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
