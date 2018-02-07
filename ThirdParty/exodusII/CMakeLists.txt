set(vtkexodusII_THIRD_PARTY 1)
set(vtkexodusII_LIBRARIES vtkexodusII)
set(vtkexodusII_INCLUDE_DIRS
  "${CMAKE_CURRENT_BINARY_DIR}/vtkexodusII/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/vtkexodusII/include")
vtk_module_export_info()
add_subdirectory(vtkexodusII)
