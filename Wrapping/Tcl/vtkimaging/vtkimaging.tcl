package require -exact vtkfiltering 4.1

if {[info commands ::vtk::init::load_source_package] != ""} {
  if {[::vtk::init::require_package vtkImagingTCL 4.1]} {
    package provide vtkimaging 4.1
  }
} else {
  if {[info commands vtkImageFFT] != "" ||
    [::vtk::load_component vtkImagingTCL] == ""} {
    package provide vtkimaging 4.1
  }
}
