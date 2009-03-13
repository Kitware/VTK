package require -exact vtkio 5.4
package require -exact vtkrendering 5.4

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkHybridTCL 5.4]} {
    package provide vtkhybrid 5.4
  }
} else {
  if {[info commands vtkEarthSource] != "" ||
    [::vtk::load_component vtkHybridTCL] == ""} {
    package provide vtkhybrid 5.4
  }
}
