package require vtkfiltering

if {[::vtk::load_component vtkIOTCL] == ""} {
    package provide vtkio 4.0
}
