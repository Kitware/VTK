package require vtkfiltering

if {[info commands vtkAxes]  != "" ||
    [::vtk::load_component vtkGraphicsTCL] == ""} {
    package provide vtkgraphics 4.0
}

