# features used by the PEGTL
target_compile_features(pegtl
  INTERFACE
    cxx_std_11)

# Not installing headers explicitly. This is not meant to be part of the VTK API.
