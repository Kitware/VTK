package require -exact vtkfiltering 4.3

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkIOTCL 4.3]} {
    package provide vtkio 4.3
  }
} else {
  if {[info commands vtkBMPReader] != "" ||
    [::vtk::load_component vtkIOTCL] == ""} {
    package provide vtkio 4.3
  }
}
