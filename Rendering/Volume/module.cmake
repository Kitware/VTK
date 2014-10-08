if (Module_vtkRenderingVolumeOpenGLNEW)
  set(VTK_RENDERING_VOLUME_BACKEND
    "vtkRenderingVolumeOpenGLNew")
endif()

vtk_module(vtkRenderingVolume
  GROUPS
    Rendering
  DEPENDS
    vtkImagingCore
    vtkRenderingCore
  TEST_DEPENDS
    vtkFiltersModeling
    vtkTestingCore
    vtkTestingRendering
    vtkRenderingVolume${VTK_RENDERING_BACKEND}
    ${VTK_RENDERING_VOLUME_BACKEND}
    vtkRenderingFreeType
    vtkIOXML
    vtkImagingSources
    vtkImagingGeneral
    vtkImagingHybrid
    vtkInteractionStyle
    vtkIOLegacy
  KIT
    vtkRendering
  )
