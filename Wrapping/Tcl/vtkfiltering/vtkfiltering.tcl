package require -exact vtkcommon 4.1

if {[info commands ::vtk::init::load_source_package] != ""} {
  if {[::vtk::init::require_package vtkFilteringTCL 4.1]} {
    package provide vtkfiltering 4.1
  }
} else {
  if {[info commands vtkCardinalSpline] != "" ||
    [::vtk::load_component vtkFilteringTCL] == ""} {
    package provide vtkfiltering 4.1
  }
}
