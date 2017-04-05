#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environement var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # April 5, 2017
  set(GIT_TAG 82917c9afd5982cfa15e4be561dd708feed28394)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKWikiExamples.git
  GIT_TAG ${GIT_TAG}
  )
