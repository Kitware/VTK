#
# Dicom Classes
#

vtk_fetch_module(vtkDICOM
  "Dicom classes and utilities"
  GIT_REPOSITORY https://github.com/dgobbi/vtk-dicom
  # vtk-dicom with override, delete for C++11 (Feb 10, 2017)
  GIT_TAG 9a4c5c3c23b952f17f2ed540cf60f932f9e1262c
  )
