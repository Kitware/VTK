#
# Add the executable
#

add_executable(vtkmyEx2 vtkmyEx2.cxx)
target_link_libraries(vtkmyEx2
  VTKMY::Unsorted
  VTK::FiltersSources VTK::RenderingCore VTK::RenderingOpenGL2)

#
# Because VTK rendering uses factory classes, it is necessary to
# declare that we want to use classes in RenderingOpenGL2 to
# implement the factory classes in RenderingCore
#

vtk_module_autoinit(
  TARGETS vtkmyEx2
  MODULES VTK::RenderingCore VTK::RenderingOpenGL2)
