package require -exact vtkio 4.3
package require -exact vtkrendering 4.3

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkParallelTCL 4.3]} {
    package provide vtkparallel 4.3
  }
} else {
  if {[info commands vtkParallelFactory] != "" ||
    [::vtk::load_component vtkParallelTCL] == ""} {
    package provide vtkparallel 4.3
  }
}
