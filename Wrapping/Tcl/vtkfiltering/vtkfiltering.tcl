package require vtkcommon

if {[::vtk::load_component vtkFilteringTCL] == ""} {
    package provide vtkfiltering 4.0
}
