package require -exact vtkio 4.2
package require -exact vtkrendering 4.2

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkHybridTCL 4.2]} {
    package provide vtkhybrid 4.2
  }
} else {
  if {[info commands vtkEarthSource] != "" ||
    [::vtk::load_component vtkHybridTCL] == ""} {
    package provide vtkhybrid 4.2
  }
}
