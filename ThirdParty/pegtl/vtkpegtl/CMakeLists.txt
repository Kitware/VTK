project (pegtl VERSION 2.4.0 LANGUAGES CXX)

# define a header-only library
add_library(vtkpegtl INTERFACE)
vtk_target_export(vtkpegtl)
vtk_target_install(vtkpegtl)

target_include_directories(vtkpegtl INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${VTK_INSTALL_INCLUDE_DIR}>
)

# features used by the PEGTL
target_compile_features(vtkpegtl INTERFACE
  cxx_alias_templates
  cxx_auto_type
  cxx_constexpr
  cxx_decltype
  cxx_default_function_template_args
  cxx_defaulted_functions
  cxx_delegating_constructors
  cxx_deleted_functions
  cxx_explicit_conversions
  cxx_generalized_initializers
  cxx_inheriting_constructors
  cxx_inline_namespaces
  cxx_noexcept
  cxx_nonstatic_member_init
  cxx_nullptr
  cxx_range_for
  cxx_rvalue_references
  cxx_static_assert
  cxx_strong_enums
  cxx_template_template_parameters
  cxx_trailing_return_types
  cxx_uniform_initialization
  cxx_variadic_macros
  cxx_variadic_templates
)

if (NOT VTK_INSTALL_NO_DEVELOPMENT)
  install(
    DIRECTORY include
    DESTINATION "${VTK_INSTALL_INCLUDE_DIR}/vtkpegtl"
    COMPONENT Development)
endif()
