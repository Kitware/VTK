if (Module_vtkRenderingVolumeOpenGLNEW AND
    NOT (VTK_RENDERING_BACKEND STREQUAL "OpenGL2"))
  set(VTK_RENDERING_VOLUME_BACKEND
    "vtkRenderingVolumeOpenGLNew")
else ()
  set (VTK_RENDERING_VOLUME_BACKEND "")
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
