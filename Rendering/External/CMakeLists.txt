set(Module_SRCS
  vtkExternalOpenGLCamera.cxx
  vtkExternalOpenGLRenderer.cxx
  vtkExternalOpenGLRenderWindow.cxx
  )

set(${vtk-module}_HDRS
  vtkExternalOpenGLCamera.h
  vtkExternalOpenGLRenderer.h
  vtkExternalOpenGLRenderWindow.h
  )

vtk_module_library(vtkRenderingExternal ${Module_SRCS})
