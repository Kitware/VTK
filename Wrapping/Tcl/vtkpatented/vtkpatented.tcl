package require -exact vtkio 4.5
package require -exact vtkrendering 4.5

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkPatentedTCL 4.5]} {
    package provide vtkpatented 4.5
  }
} else {
  if {[info commands vtkKitwareContourFilter] != "" ||
    [::vtk::load_component vtkPatentedTCL] == ""} {
    package provide vtkpatented 4.5
  }
}
