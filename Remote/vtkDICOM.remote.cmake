#
# Dicom Classes
#

vtk_fetch_module(vtkDICOM
  "Dicom classes and utilities"
  GIT_REPOSITORY https://github.com/dgobbi/vtk-dicom
  # vtk-dicom with first batch of VTK 8.90 fixes
  GIT_TAG fa403f6ee5201f4ff49ee866a843e69f86233cb1
  )
