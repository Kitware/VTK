package require -exact vtkfiltering 4.5

if {[info commands ::vtk::init::require_package] != ""} {
  if {[::vtk::init::require_package vtkGraphicsTCL 4.5]} {
    package provide vtkgraphics 4.5
  }
} else {
  if {[info commands vtkAxes]  != "" ||
    [::vtk::load_component vtkGraphicsTCL] == ""} {
    package provide vtkgraphics 4.5
  }
}
