package require -exact vtkcommon 4.2

foreach s {colors mccases backdrop grab} {
  source [file join [file dirname [info script]] "${s}.tcl"]
}

# Invoke DeleteAllObjects on exit

rename ::exit ::vtk::exit
proc ::exit {{returnCode 0}} {
    vtkCommand DeleteAllObjects
    return [::vtk::exit $returnCode]
}

package provide vtktesting 4.2
