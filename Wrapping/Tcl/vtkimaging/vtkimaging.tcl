package require -exact vtkfiltering 4.5

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkImagingTCL 4.5]} {
    package provide vtkimaging 4.5
  }
} else {
  if {[info commands vtkImageFFT] != "" ||
    [::vtk::load_component vtkImagingTCL] == ""} {
    package provide vtkimaging 4.5
  }
}
