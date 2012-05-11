package require vtk
package require vtkinteraction
package require vtktesting

# Load base (spike and test)

source [file join [file dirname [info script]] TestStyleBase.tcl]
source [file join [file dirname [info script]] TestStyleBaseSpike.tcl]

# Set interactor style

vtkInteractorStyleTerrain inStyle
iren SetInteractorStyle inStyle
iren Initialize

# Test style

test_style inStyle
