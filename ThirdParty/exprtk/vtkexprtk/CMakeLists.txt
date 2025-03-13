set(headers
 include/exprtk.hpp)

vtk_module_install_headers(
  USE_RELATIVE_PATHS
  SUBDIR  "vtkexprtk"
  FILES   ${headers})
