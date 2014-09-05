set(VTK_VOLUMEOPENGL2)

if (Module_vtkVolumeOpenGL2)
  set(VTK_VOLUMEOPENGL2 vtkVolumeOpenGL2)
endif()

vtk_module(vtkRenderingVolumeOpenGL
  IMPLEMENTS
    vtkRenderingVolume
  BACKEND
    OpenGL
  DEPENDS
    vtkRenderingOpenGL
    ${VTK_VOLUMEOPENGL2}
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
