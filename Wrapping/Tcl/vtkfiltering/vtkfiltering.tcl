package require -exact vtkcommon 4.5

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkFilteringTCL 4.5]} {
    package provide vtkfiltering 4.5
  }
} else {
  if {[info commands vtkCardinalSpline] != "" ||
    [::vtk::load_component vtkFilteringTCL] == ""} {
    package provide vtkfiltering 4.5
  }
}
