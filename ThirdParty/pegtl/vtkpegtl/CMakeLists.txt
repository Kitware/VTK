# features used by the PEGTL
vtk_module_compile_features(VTK::pegtl
  INTERFACE
    cxx_std_11)

# Not installing headers explicitly. This is not meant to be part of the VTK API.
