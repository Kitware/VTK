vtk_module_third_party(pugixml
  LIBRARIES vtkpugixml
  INCLUDE_DIRS ${vtkpugixml_SOURCE_DIR}/vtkpugixml
  ${vtkpugixml_BINARY_DIR}/vtkpugixml
  NO_ADD_SUBDIRECTORY)

vtk_module_export_info()
add_subdirectory(vtkpugixml)

vtk_target_export(vtkpugixml)
