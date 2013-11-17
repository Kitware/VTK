# This module simply exists to provide unified
# install rules for all Web group modules
vtk_module(vtkWebInstall
  GROUPS
    Web
  EXCLUDE_FROM_WRAPPING
  EXCLUDE_FROM_ALL)
