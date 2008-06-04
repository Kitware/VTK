package require -exact vtkwidgets 5.3

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkGeovisTCL 5.3]} {
    package provide vtkgeovis 5.3
  }
} else {
  if {[info commands /* vtkGraphLayout */] != "" ||
    [::vtk::load_component vtkGeovisTCL] == ""} {
    package provide vtkgeovis 5.3
  }
}
