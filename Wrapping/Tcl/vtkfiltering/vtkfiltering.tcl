package require vtkcommon

if {[info commands vtkCardinalSpline] != "" ||
    [::vtk::load_component vtkFilteringTCL] == ""} {
    package provide vtkfiltering 4.0
}
