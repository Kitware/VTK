#
# Midas Journal 838
#

vtk_fetch_module(SplineDrivenImageSlicer
  "Spline Driven Image Slicer - http://www.vtkjournal.org/browse/publication/838"

# FIXME lorensen's repo needs to accept merge request #2 for new vtkCellArray
# API support. Once that happens, this url should be switched back and the
# tag updated. This change is intended to only be temporary so that Kitware's
# CI builds pass.

#  GIT_REPOSITORY https://github.com/lorensen/midas-journal-838.git
  GIT_REPOSITORY https://github.com/allisonvacanti/midas-journal-838.git

  GIT_TAG 281f1adfb681f2fce8be00dda0e07b3b92dc7939
  )
