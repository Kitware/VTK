# FIXME: jsoncpp only exports symbols properly on Windows.
unset(CMAKE_CXX_VISIBILITY_PRESET)
unset(CMAKE_VISIBILITY_INLINES_HIDDEN)

vtk_module_third_party(
  INTERNAL
    LICENSE_FILES "vtkjsoncpp/LICENSE"
    VERSION       "1.9.2"
    STANDARD_INCLUDE_DIRS
  EXTERNAL
    PACKAGE JsonCpp
    VERSION 0.7.0
    TARGETS JsonCpp::JsonCpp
    STANDARD_INCLUDE_DIRS)

if (VTK_MODULE_USE_EXTERNAL_vtkjsoncpp)
  if (EXISTS "${JsonCpp_INCLUDE_DIR}/json/json_forwards.h")
    set (JsonCpp_FORWARDING_HEADER_NAME "json_forwards.h")
  else ()
    set (JsonCpp_FORWARDING_HEADER_NAME "forwards.h")
  endif ()
  include(vtkDetectLibraryType)
  vtk_detect_library_shared(vtkjsoncpp JsonCpp::JsonCpp)
else ()
  set(vtkjsoncpp_is_shared "${BUILD_SHARED_LIBS}")
endif ()

set(VTK_JSONCPP_SHARED "${vtkjsoncpp_is_shared}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_jsoncpp.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_jsoncpp.h")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/vtk_jsoncpp_fwd.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/vtk_jsoncpp_fwd.h")

vtk_module_install_headers(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/vtk_jsoncpp.h"
        "${CMAKE_CURRENT_BINARY_DIR}/vtk_jsoncpp_fwd.h")
