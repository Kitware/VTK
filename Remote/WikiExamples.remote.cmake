#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environment var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # December 18 Baseline additions
  set(GIT_TAG 94c37e7520086d88dd6774c3943e749b7bd0a76c)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKExamples.git
  GIT_TAG ${GIT_TAG}
  )
