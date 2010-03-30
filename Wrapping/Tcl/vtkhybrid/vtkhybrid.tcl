package require -exact vtkio 5.7
package require -exact vtkrendering 5.7

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkHybridTCL 5.7]} {
    package provide vtkhybrid 5.7
  }
} else {
  if {[info commands vtkEarthSource] != "" ||
    [::vtk::load_component vtkHybridTCL] == ""} {
    package provide vtkhybrid 5.7
  }
}
