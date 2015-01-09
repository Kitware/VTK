# This module cannot be built when the rendering backend is OpenGL2
if (Module_vtkRenderingExternal AND
    ("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2"))
  message (SEND_ERROR "
  vtkRenderingExternal cannot be built
  when the rendering backend is OpenGL2.
  Please disable vtkRenderingExternal or
  change VTK_RENDERING_BACKEND to OpenGL.
  ")
  return()
endif()

vtk_module(vtkRenderingExternal
  DEPENDS
    vtkRenderingCore
    vtkRenderingOpenGL
  TEST_DEPENDS
    vtkTestingRendering
  EXCLUDE_FROM_ALL
  )
