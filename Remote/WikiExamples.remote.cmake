#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environement var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # April 19, 2016
  set(GIT_TAG 533e7047fc8bbec29cfe82686cd66eec9b94ba0c)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKWikiExamples.git
  GIT_TAG ${GIT_TAG}
  )
