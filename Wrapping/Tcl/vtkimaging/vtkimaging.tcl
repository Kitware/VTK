package require -exact vtkfiltering 4.3

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkImagingTCL 4.3]} {
    package provide vtkimaging 4.3
  }
} else {
  if {[info commands vtkImageFFT] != "" ||
    [::vtk::load_component vtkImagingTCL] == ""} {
    package provide vtkimaging 4.3
  }
}
