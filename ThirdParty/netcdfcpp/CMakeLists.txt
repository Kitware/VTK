vtk_module_third_party(NetCDFCPP
  PACKAGE NetCDF
  COMPONENTS CXX)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_netcdfcpp_fwd.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdfcpp_fwd.h")
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(
    FILES       "${CMAKE_CURRENT_BINARY_DIR}/vtk_netcdfcpp_fwd.h"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}"
    COMPONENT   Development)
endif ()
