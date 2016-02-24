set(VTK_RENDERINGVOLUMEOPENGLNEW)

if (Module_vtkRenderingVolumeOpenGLNew AND
    NOT (VTK_RENDERING_BACKEND STREQUAL "OpenGL2"))
  set (VTK_RENDERINGVOLUMEOPENGLNEW vtkRenderingVolumeOpenGLNew)
else ()
  set (VTK_RENDERINGVOLUMEOPENGLNEW "")
endif()

vtk_module(vtkRenderingVolumeOpenGL
  IMPLEMENTS
    vtkRenderingVolume
  BACKEND
    OpenGL
  IMPLEMENTATION_REQUIRED_BY_BACKEND
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
