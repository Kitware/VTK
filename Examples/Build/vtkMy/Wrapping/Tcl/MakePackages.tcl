#!/usr/bin/env tclsh

# Execute this script each time you add a new directory or file.

# Packages

pkg_mkIndex -direct -verbose vtkmy
pkg_mkIndex -direct -verbose vtkmycommon
pkg_mkIndex -direct -verbose vtkmyimaging
pkg_mkIndex -direct -verbose vtkmyunsorted

exit