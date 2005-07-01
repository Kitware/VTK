package require -exact vtkhybrid 4.5

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkWidgetsTCL 4.5]} {
    package provide vtkwidgets 4.5
  }
} else {
  if {[info commands vtkBorderWidget] != "" ||
    [::vtk::load_component vtkWidgetsTCL] == ""} {
    package provide vtkwidgets 4.5
  }
}
