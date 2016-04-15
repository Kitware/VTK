set(VTK_RENDERINGVOLUMEOPENGLNEW)

if (Module_vtkRenderingVolumeOpenGLNew AND
    NOT (VTK_RENDERING_BACKEND STREQUAL "OpenGL2"))
  set (VTK_RENDERINGVOLUMEOPENGLNEW vtkRenderingVolumeOpenGLNew)
else ()
  set (VTK_RENDERINGVOLUMEOPENGLNEW "")
endif()

set(ospray_depend "")
if (VTK_USE_OSPRAY)
  set(ospray_depend vtkRenderingOSPRay)
endif (VTK_USE_OSPRAY)

vtk_module(vtkRenderingVolumeOpenGL
  IMPLEMENTS
    vtkRenderingVolume
  BACKEND
    OpenGL
  IMPLEMENTATION_REQUIRED_BY_BACKEND
  DEPENDS
    vtkRenderingOpenGL
    ${VTK_RENDERINGVOLUMEOPENGLNEW}
    ${ospray_depend}
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
