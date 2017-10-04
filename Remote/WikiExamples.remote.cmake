#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environement var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # October 3, 2017 VTKBookExamples alpha
  set(GIT_TAG 5306688599794ccd78367f33d1662c2a1b14b718)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKExamples.git
  GIT_TAG ${GIT_TAG}
  )
