package require -exact vtkio 4.4
package require -exact vtkrendering 4.4

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkPatentedTCL 4.4]} {
    package provide vtkpatented 4.4
  }
} else {
  if {[info commands vtkKitwareContourFilter] != "" ||
    [::vtk::load_component vtkPatentedTCL] == ""} {
    package provide vtkpatented 4.4
  }
}
