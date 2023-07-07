vtk_module_third_party(
  INTERNAL
    LICENSE_FILES
      "vtknetcdf/COPYRIGHT"
    SPDX_LICENSE_IDENTIFIER
      "BSD-3-Clause"
    SPDX_COPYRIGHT_TEXT
      "Copyright 2018 Unidata"
    SPDX_DOWNLOAD_LOCATION
      "git+https://gitlab.kitware.com/third-party/netcdf.git@for/vtk-20230505-4.9.2"
    VERSION
      "4.9.2"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE NetCDF
    TARGETS NetCDF::NetCDF
    USE_VARIABLES NetCDF_HAS_PARALLEL
    STANDARD_INCLUDE_DIRS)

set(vtk_netcdf_has_parallel FALSE)
if (VTK_MODULE_USE_EXTERNAL_vtknetcdf AND NetCDF_HAS_PARALLEL AND TARGET VTK::mpi)
  vtk_module_link(VTK::netcdf
    INTERFACE
      VTK::mpi)
  set(vtk_netcdf_has_parallel TRUE)
endif()

# set a property to let dependencies know that NetCDF has
# parallel API
set_property(GLOBAL
  PROPERTY
    "_vtk_netcdf_has_parallel" "${vtk_netcdf_has_parallel}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_netcdf.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdf.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdf.h")
