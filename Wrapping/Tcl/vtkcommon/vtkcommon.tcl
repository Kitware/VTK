package require vtkbase

if {[info commands vtkObject] != "" ||
    [::vtk::load_component vtkCommonTCL] == ""} {
    package provide vtkcommon 4.0

    # Invoke DeleteAllObjects on exit

    rename ::exit ::vtk::exit
    proc ::exit {{returnCode 0}} {
        vtkCommand DeleteAllObjects
        return [::vtk::exit $returnCode]
    }
}