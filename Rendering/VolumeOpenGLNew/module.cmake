# This module cannot be built when the rendering backend is OpenGL2
if (Module_vtkRenderingVolumeOpenGLNew AND
    ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2"))
  message (SEND_ERROR "
  vtkRenderingVolumeOpenGLNew cannot be built
  when the rendering backend is OpenGL2.
  Please disable vtkRenderingVolumeOpenGLNew or
  change VTK_RENDERING_BACKEND to OpenGL.
  ")
  return()
endif()

vtk_module(vtkRenderingVolumeOpenGLNew
  DEPENDS
    vtkCommonCore
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkFiltersSources
    vtkglew
    vtkRenderingOpenGL
    vtkRenderingVolume
  IMPLEMENTS
    vtkRenderingVolume
  TEST_DEPENDS
    vtkCommonCore
    vtkFiltersModeling
    vtkglew
    vtkInteractionStyle
    vtkIOLegacy
    vtkIOXML
    vtkRenderingOpenGL
    vtkTestingCore
    vtkTestingRendering
  EXCLUDE_FROM_ALL
)
