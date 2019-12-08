#
# Dicom Classes
#

vtk_fetch_module(vtkDICOM
  "Dicom classes and utilities"
  GIT_REPOSITORY https://github.com/dgobbi/vtk-dicom
  # vtk-dicom release 8.9.12 plus undef warning fix
  GIT_TAG 8a337caf3765d0127e2dbc450834c8d5215f8b78
  )
