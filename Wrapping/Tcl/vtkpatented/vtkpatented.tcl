package require -exact vtkio 4.3
package require -exact vtkrendering 4.3

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkPatentedTCL 4.3]} {
    package provide vtkpatented 4.3
  }
} else {
  if {[info commands vtkKitwareContourFilter] != "" ||
    [::vtk::load_component vtkPatentedTCL] == ""} {
    package provide vtkpatented 4.3
  }
}
