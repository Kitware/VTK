package require vtkcommon

package provide vtktesting 4.0

# Invoke DeleteAllObjects on exit

rename ::exit ::vtk::exit
proc ::exit {{returnCode 0}} {
    vtkCommand DeleteAllObjects
    return [::vtk::exit $returnCode]
}
