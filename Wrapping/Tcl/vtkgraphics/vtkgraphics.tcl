package require -exact vtkfiltering 4.1

if {[info commands ::vtk::init::load_source_package] != ""} {
  if {[::vtk::init::require_package vtkGraphicsTCL 4.1]} {
    package provide vtkgraphics 4.1
  }
} else {
  if {[info commands vtkAxes]  != "" ||
    [::vtk::load_component vtkGraphicsTCL] == ""} {
    package provide vtkgraphics 4.1
  }
}
