package require vtkfiltering

if {[::vtk::load_component vtkGraphicsTCL] == ""} {
    package provide vtkgraphics 4.0
}

