package require vtkbase

if {[::vtk::load_component vtkCommonTCL] == ""} {
    package provide vtkcommon 4.0
}