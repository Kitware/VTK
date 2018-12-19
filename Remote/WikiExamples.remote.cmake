#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environment var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # December 19 Baseline additions
  set(GIT_TAG 744133bb3c344e09817ca27e3a10e3e0f2a02e11)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKExamples.git
  GIT_TAG ${GIT_TAG}
  )
