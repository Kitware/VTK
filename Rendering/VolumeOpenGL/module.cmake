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
  TEST_DEPENDS
    vtkTestingCore
    vtkTestingRendering
    vtkRenderingFreeType
    vtkImagingSources
    vtkImagingGeneral
    vtkInteractionStyle
  KIT
    vtkOpenGL
  DEPENDS
    ${VTK_RENDERINGVOLUMEOPENGLNEW}
    vtkCommonCore
    vtkImagingCore
    vtkRenderingOpenGL
    vtkRenderingVolume
  PRIVATE_DEPENDS
    vtkCommonDataModel
    vtkCommonMath
    vtkCommonSystem
    vtkCommonTransforms
    vtkFiltersCore
    vtkFiltersGeneral
    vtkFiltersGeometry
    vtkFiltersSources
    vtkRenderingCore
    vtksys
  )