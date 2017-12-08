#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environement var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # December 8, 2017 Address VTK api changes
  set(GIT_TAG 9447dfa6f56dc2a958bbabf4f679c20230f90a8f)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKExamples.git
  GIT_TAG ${GIT_TAG}
  )
