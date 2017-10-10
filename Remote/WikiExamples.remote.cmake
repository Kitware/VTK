#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environement var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # October 9, 2017 New alternative baselines
  set(GIT_TAG b73b03bc4bdd7d0053e0c2a5322a94a2bef937d8)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKExamples.git
  GIT_TAG ${GIT_TAG}
  )
