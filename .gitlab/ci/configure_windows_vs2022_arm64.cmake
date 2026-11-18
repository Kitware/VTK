set(VTK_MODULE_ENABLE_VTK_RenderingOpenXR NO CACHE STRING "") # OpenXR disable on Windows Arm
include("${CMAKE_CURRENT_LIST_DIR}/configure_windows.cmake")
