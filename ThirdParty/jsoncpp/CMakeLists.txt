include(CMakeDependentOption)

vtk_module_third_party(JsonCpp
  VERSION 0.7.0)

cmake_dependent_option(VTK_SYSTEM_JSONCPP_SHARED "Whether the system jsoncpp is shared or not" ON VTK_USE_SYSTEM_JSONCPP "${BUILD_SHARED_LIBS}")

if (VTK_USE_SYSTEM_JSONCPP)
  if (EXISTS "${JsonCpp_INCLUDE_DIR}/json/json_forwards.h")
    set (JsonCpp_FORWARDING_HEADER_NAME "json_forwards.h")
  else ()
    set (JsonCpp_FORWARDING_HEADER_NAME "forwards.h")
  endif ()
endif ()

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_jsoncpp_fwd.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_jsoncpp_fwd.h")
if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(
    FILES       "${CMAKE_CURRENT_BINARY_DIR}/vtk_jsoncpp_fwd.h"
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}"
    COMPONENT   Development)
endif ()
