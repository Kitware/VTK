#
# Dicom Classes
#

vtk_fetch_module(vtkDICOM
  "Dicom classes and utilities"
  GIT_REPOSITORY https://github.com/dgobbi/vtk-dicom
  # vtk-dicom with second batch of VTK 8.90 fixes
  GIT_TAG 5bf860c73579a142d2bd667600a178ac153e4fc9
  )
