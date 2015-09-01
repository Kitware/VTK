set(Module_SRCS
  vtkGL2PSContextDevice2D.cxx
  vtkGL2PSUtilities.cxx
  )
set_source_files_properties(
  vtkGL2PSContextDevice2D
  WRAP_EXCLUDE)
vtk_module_library(${vtk-module} ${Module_SRCS})
include(vtkOpenGL)
vtk_opengl_link(${vtk-module})
