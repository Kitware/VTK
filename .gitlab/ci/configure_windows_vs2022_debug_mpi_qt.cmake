# We use release Python builds (if Python is available).
set(VTK_WINDOWS_PYTHON_DEBUGGABLE OFF CACHE BOOL "")

# Enable all dispatch arrays. Windows Debug builds have historically had
# problems with template instantiations.
set(VTK_DISPATCH_AFFINE_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_AOS_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_COMPOSITE_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_CONSTANT_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_IMPLICIT_POINT_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_INDEXED_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_SOA_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_STD_FUNCTION_ARRAYS ON CACHE BOOL "")
set(VTK_DISPATCH_STRUCTURED_POINT_ARRAYS ON CACHE BOOL "")
# `vtkHyperTreeGridContour` uses `vtkNew<ArrayType>` on dispatched types but
# `vtkTypedDataArray` is an abstract type and cannot be instantiated in this
# way. Until this is fixed (#19213), disable dispatching even though the
# template instantiation logic is more important for this CI configuration
# because `VTK::TestingCore` needs `VTK::FiltersHyperTree`.
set(VTK_DISPATCH_TYPED_ARRAYS OFF CACHE BOOL "")

# Disable VTK-m as the artifacts are too large.
set(VTK_MODULE_ENABLE_VTK_vtkvtkm NO CACHE STRING "")

include("${CMAKE_CURRENT_LIST_DIR}/configure_windows.cmake")
