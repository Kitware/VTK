#
# Dicom Classes
#

vtk_fetch_module(vtkDICOM
  "Dicom classes and utilities"
  GIT_REPOSITORY https://github.com/dgobbi/vtk-dicom
  # vtk-dicom with first batch of VTK 8.90 fixes
  GIT_TAG 2538a7ad336a0afe8e2bffb4e36d2a555cd910ce
  )
