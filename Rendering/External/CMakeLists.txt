set(Module_SRCS
  ExternalVTKWidget.cxx
  vtkExternalOpenGLCamera.cxx
  vtkExternalOpenGLRenderer.cxx
  vtkExternalOpenGLRenderWindow.cxx
  )

set(${vtk-module}_HDRS
  ExternalVTKWidget.h
  vtkExternalOpenGLCamera.h
  vtkExternalOpenGLRenderer.h
  vtkExternalOpenGLRenderWindow.h
  )

vtk_module_library(vtkRenderingExternal ${Module_SRCS})
