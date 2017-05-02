vtk_module_third_party(NetCDFCPP
  PACKAGE NetCDF
  COMPONENTS CXX)

if (VTK_USE_SYSTEM_NETCDFCPP AND NOT VTK_USE_SYSTEM_NETCDF)
  message(FATAL_ERROR
    "Using a system netcdfcpp with VTK's netcdf is not supported. Please set "
    "VTK_USE_SYSTEM_NETCDF as well so that the same netcdf library is used "
    "everywhere in VTK.")
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_netcdfcpp_fwd.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdfcpp_fwd.h")
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(
    FILES       "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdfcpp_fwd.h"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}"
    COMPONENT   Development)
endif ()
