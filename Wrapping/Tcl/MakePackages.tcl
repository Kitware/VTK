#!/usr/bin/env tclsh

# Packages

pkg_mkIndex -direct -verbose vtk
pkg_mkIndex -direct -verbose vtkbase
pkg_mkIndex -direct -verbose vtkcommon 
pkg_mkIndex -direct -verbose vtkfiltering
pkg_mkIndex -direct -verbose vtkgraphics
pkg_mkIndex -direct -verbose vtkhybrid
pkg_mkIndex -direct -verbose vtkimaging
pkg_mkIndex -direct -verbose vtkio
pkg_mkIndex -direct -verbose vtkparallel
pkg_mkIndex -direct -verbose vtkpatented
pkg_mkIndex -direct -verbose vtkrendering

# Add-ons (scripts)

pkg_mkIndex -direct -verbose vtkinteraction
pkg_mkIndex -direct -verbose vtktesting

exit