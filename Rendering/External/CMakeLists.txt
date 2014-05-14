set(Module_SRCS
  vtkExternalOpenGLRenderWindow.cxx
  )

set(${vtk-module}_HDRS
  vtkExternalOpenGLRenderWindow.h
  )

vtk_module_library(vtkRenderingExternal ${Module_SRCS})
