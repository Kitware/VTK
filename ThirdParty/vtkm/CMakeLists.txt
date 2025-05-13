vtk_module_add_module(VTK::vtkvtkm HEADER_ONLY)
vtk_module_link(VTK::vtkvtkm INTERFACE VTK::viskores)

set(deprecation_warning "Target VTK::vtkvtkm deprecated, VTK::vtkviskores enabled.")
message(DEPRECATION "${deprecation_warning}")

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.17")
  set_target_properties(vtkvtkm PROPERTIES DEPRECATION "${deprecation_warning}")
endif()
