package require -exact vtkio 4.1
package require -exact vtkrendering 4.1

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkParallelTCL 4.1]} {
    package provide vtkparallel 4.1
  }
} else {
  if {[info commands vtkParallelFactory] != "" ||
    [::vtk::load_component vtkParallelTCL] == ""} {
    package provide vtkparallel 4.1
  }
}
