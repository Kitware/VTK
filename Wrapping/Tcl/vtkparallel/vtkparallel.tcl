package require -exact vtkio 4.2
package require -exact vtkrendering 4.2

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkParallelTCL 4.2]} {
    package provide vtkparallel 4.2
  }
} else {
  if {[info commands vtkParallelFactory] != "" ||
    [::vtk::load_component vtkParallelTCL] == ""} {
    package provide vtkparallel 4.2
  }
}
