package require -exact vtkwidgets 5.1

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkViewsTCL 5.1]} {
    package provide vtkviews 5.1
  }
} else {
  if {[info commands vtkGraphLayout] != "" ||
    [::vtk::load_component vtkViewsTCL] == ""} {
    package provide vtkviews 5.1
  }
}
