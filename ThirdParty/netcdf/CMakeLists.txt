vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtknetcdf/COPYRIGHT"
    VERSION       "4.7.3"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE NetCDF
    TARGETS NetCDF::NetCDF
    STANDARD_INCLUDE_DIRS)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_netcdf.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdf.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdf.h")
