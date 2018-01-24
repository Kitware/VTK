#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environment var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # January 24, 2018 Time for an update
  set(GIT_TAG edb9d2e8ed530ed96f7e5625751b1a051d96a423)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKExamples.git
  GIT_TAG ${GIT_TAG}
  )
