package require -exact vtkwidgets 5.4

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkViewsTCL 5.4]} {
    package provide vtkviews 5.4
  }
} else {
  if {[info commands vtkGraphLayout] != "" ||
    [::vtk::load_component vtkViewsTCL] == ""} {
    package provide vtkviews 5.4
  }
}
