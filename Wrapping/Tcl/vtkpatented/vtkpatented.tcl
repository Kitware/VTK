package require -exact vtkio 4.1
package require -exact vtkrendering 4.1

if {[info commands ::vtk::init::load_source_package] != ""} {
  if {[::vtk::init::require_package vtkPatentedTCL 4.1]} {
    package provide vtkpatented 4.1
  }
} else {
  if {[info commands vtkKitwareContourFilter] != "" ||
    [::vtk::load_component vtkPatentedTCL] == ""} {
    package provide vtkpatented 4.1
  }
}
