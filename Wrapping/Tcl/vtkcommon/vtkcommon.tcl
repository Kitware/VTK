package require -exact vtkbase 4.1

if {[info commands ::vtk::init::load_source_package] != ""} {
  if {[::vtk::init::require_package vtkCommonTCL 4.1]} {
    package provide vtkcommon 4.1
  }
} else {
  if {[info commands vtkObject] != "" ||
    [::vtk::load_component vtkCommonTCL] == ""} {
    package provide vtkcommon 4.1
  }
}
