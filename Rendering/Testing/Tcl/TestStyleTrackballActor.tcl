package require vtk
package require vtkinteraction
package require vtktesting

# Load base (spike and test)

source [file join [file dirname [info script]] TestStyleBase.tcl]
source [file join [file dirname [info script]] TestStyleBaseSpike.tcl]

# Set interactor style

vtkInteractorStyleSwitch inStyle
iren SetInteractorStyle inStyle
iren Initialize

# Switch to Trackball+Actor mode

iren SetKeyEventInformation 0 0 t 0 0
iren InvokeEvent "CharEvent"

iren SetKeyEventInformation 0 0 a 0 0
iren InvokeEvent "CharEvent"

# Test style

test_style [inStyle GetCurrentStyle]
