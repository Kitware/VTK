foreach kit ${vtk::init::kits} {
  package require -exact vtk${kit} 4.5
}

# Invoke DeleteAllObjects on exit

rename ::exit ::vtk::exit
proc ::exit {{returnCode 0}} {
    vtkCommand DeleteAllObjects
    return [::vtk::exit $returnCode]
}

package provide vtk 4.5
