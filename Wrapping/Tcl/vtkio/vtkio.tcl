package require vtkfiltering

if {[info commands vtkBMPReader] != "" ||
    [::vtk::load_component vtkIOTCL] == ""} {
    package provide vtkio 4.0
}
