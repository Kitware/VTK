package require vtkcommon

if {[info commands vtkDataObjectSource] != "" ||
    [::vtk::load_component vtkFilteringTCL] == ""} {
    package provide vtkfiltering 4.0
}
