vtk_module_third_party(JsonCpp
  VERSION 0.7.0)
if (WIN32 AND VTK_USE_SYSTEM_JSONCPP)
  option(VTK_SYSTEM_JSONCPP_SHARED "System-installed JsonCpp package is a shared library" OFF)
  mark_as_advanced(VTK_SYSTEM_JSONCPP_SHARED)
endif()
