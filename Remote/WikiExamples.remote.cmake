#
# VTK WikiExamples
#
# To run tests for this module
# ctest -L WikiExamples

# If the environment var WikiExamplesTag exists, use it
if (NOT DEFINED ENV{WikiExamplesTag})
  # 22 March 2018 Added Shader Examples
  set(GIT_TAG 85691c79381a90c11ad90f28a3df3cfae3dcc708)
else()
  set(GIT_TAG $ENV{WikiExamplesTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKExamples.git
  GIT_TAG ${GIT_TAG}
  )
