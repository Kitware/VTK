# features used by the PEGTL
target_compile_features(pegtl
  INTERFACE
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
    cxx_variadic_templates)

# Not installing headers explicitly. This is not meant to be part of the VTK API.
