package require -exact vtkfiltering 4.1

if {[info commands ::vtk::init::load_source_package] != ""} {
  if {[::vtk::init::require_package vtkIOTCL 4.1]} {
    package provide vtkio 4.1
  }
} else {
  if {[info commands vtkBMPReader] != "" ||
    [::vtk::load_component vtkIOTCL] == ""} {
    package provide vtkio 4.1
  }
}
