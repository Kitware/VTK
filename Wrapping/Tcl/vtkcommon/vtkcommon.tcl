package require vtkbase

if {[info commands vtkObject] != "" ||
    [::vtk::load_component vtkCommonTCL] == ""} {
    package provide vtkcommon 4.0
}