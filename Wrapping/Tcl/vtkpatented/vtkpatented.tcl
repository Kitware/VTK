package require -exact vtkio 4.2
package require -exact vtkrendering 4.2

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkPatentedTCL 4.2]} {
    package provide vtkpatented 4.2
  }
} else {
  if {[info commands vtkKitwareContourFilter] != "" ||
    [::vtk::load_component vtkPatentedTCL] == ""} {
    package provide vtkpatented 4.2
  }
}
