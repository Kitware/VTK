#
# VTK WikiExamples
#

# If the environement var RemoteGitTag exists, use it
if (NOT DEFINED ENV{RemoteGitTag})
  # February 17, 2016
  # Qt5 fixes
  set(GIT_TAG 184ef64f241c53356a8a6c3852dfb2ec18691d49)
else()
  set(GIT_TAG $ENV{RemoteGitTag})
endif()

vtk_fetch_module(WikiExamples
  "A collection of examples that illustrate how to use VTK."
  GIT_REPOSITORY https://github.com/lorensen/VTKWikiExamples.git
  # February 14, 2016
  # New baselines
  GIT_TAG ${GIT_TAG}
  )
