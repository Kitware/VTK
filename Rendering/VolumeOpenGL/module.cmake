set(VTK_RENDERINGVOLUMEOPENGLNEW)

if (Module_vtkRenderingVolumeOpenGLNew)
  set(VTK_RENDERINGVOLUMEOPENGLNEW vtkRenderingVolumeOpenGLNew)
endif()

vtk_module(vtkRenderingVolumeOpenGL
  IMPLEMENTS
    vtkRenderingVolume
  BACKEND
    OpenGL
  DEPENDS
    vtkRenderingOpenGL
    ${VTK_RENDERINGVOLUMEOPENGLNEW}
  PRIVATE_DEPENDS
    vtksys
    vtkFiltersGeneral
    vtkFiltersSources
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkRenderingFreeType
    vtkImagingSources
    vtkImagingGeneral
    vtkInteractionStyle
  KIT
    vtkOpenGL
  )
