package require -exact vtkfiltering 4.3

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkGraphicsTCL 4.3]} {
    package provide vtkgraphics 4.3
  }
} else {
  if {[info commands vtkAxes]  != "" ||
    [::vtk::load_component vtkGraphicsTCL] == ""} {
    package provide vtkgraphics 4.3
  }
}
