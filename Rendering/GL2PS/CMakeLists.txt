set(Module_SRCS
  vtkGL2PSContextDevice2D.cxx
  vtkGL2PSUtilities.cxx
  )
set_source_files_properties(
  vtkGL2PSContextDevice2D
  WRAP_EXCLUDE)


include_directories(${OPENGL_INCLUDE_DIR})

vtk_module_library(${vtk-module} ${Module_SRCS})
