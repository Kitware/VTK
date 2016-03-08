#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environement var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # February 29, 2016
  set(GIT_TAG ab816c494f60b7e94c992e9e5dabcadd28227dc1)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKWikiExamples.git
  # February 14, 2016
  # New baselines
  GIT_TAG ${GIT_TAG}
  )
