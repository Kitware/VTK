package require -exact vtkio 4.5
package require -exact vtkrendering 4.5

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkHybridTCL 4.5]} {
    package provide vtkhybrid 4.5
  }
} else {
  if {[info commands vtkEarthSource] != "" ||
    [::vtk::load_component vtkHybridTCL] == ""} {
    package provide vtkhybrid 4.5
  }
}
