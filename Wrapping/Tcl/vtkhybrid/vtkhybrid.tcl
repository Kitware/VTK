package require -exact vtkio 4.1
package require -exact vtkrendering 4.1

if {[info commands ::vtk::init::load_source_package] != ""} {
  if {[::vtk::init::require_package vtkHybridTCL 4.1]} {
    package provide vtkhybrid 4.1
  }
} else {
  if {[info commands vtkEarthSource] != "" ||
    [::vtk::load_component vtkHybridTCL] == ""} {
    package provide vtkhybrid 4.1
  }
}
