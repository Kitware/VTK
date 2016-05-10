set(optional_test_depends)
if("${Module_vtkRenderingParallel}" AND "${Module_vtkParallelMPI}")
  set(optional_test_depends "vtkRenderingParallel;vtkParallelMPI")
endif()

vtk_module(vtkRenderingOSPRay
  DEPENDS
    vtkRenderingVolume
    vtkRenderingSceneGraph
    #todo promote compositedatadisplayattributes to rendering/core
    vtkRendering${VTK_RENDERING_BACKEND} #only for comp.data.disp.attr.
  IMPLEMENTS
    vtkRenderingVolume
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
    # Dependencies for volume rendering tests
    vtkRenderingVolume
    vtkRenderingVolume${VTK_RENDERING_BACKEND}
    vtkFiltersCore
    vtkFiltersHybrid
    vtkFiltersModeling
    vtkImagingSources
    vtkImagingGeneral
    vtkImagingHybrid
    vtkInteractionStyle
    vtkIOLegacy
    vtkRenderingFreeType
  EXCLUDE_FROM_ALL
  )
