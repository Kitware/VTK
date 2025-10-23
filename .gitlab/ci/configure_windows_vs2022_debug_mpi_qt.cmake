# We use release Python builds (if Python is available).
set(VTK_WINDOWS_PYTHON_DEBUGGABLE OFF CACHE BOOL "")

# Enable all dispatch arrays. Windows Debug builds have historically had
# problems with template instantiations.
set(VTK_DISPATCH_AFFINE_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_AOS_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_CONSTANT_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_SCALED_SOA_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_SOA_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_STD_FUNCTION_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_STRUCTURED_POINT_ARRAYS ON CACHE BOOL "")

# Disable viskores as the artifacts are too large.
set(VTK_MODULE_ENABLE_VTK_vtkviskores NO CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_windows.cmake")
