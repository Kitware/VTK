package require vtk
package require vtkinteraction

# Load base (spike and test)

source [file join [file dirname [info script]] TestStyleBase.tcl]
source [file join [file dirname [info script]] TestStyleBaseSpike.tcl]

# Set interactor style

vtkInteractorStyleRubberBandZoom inStyle
iren SetInteractorStyle inStyle
iren Initialize

# Test style

iren SetEventInformationFlipY 150 150 0 0 0 0 0
iren InvokeEvent "LeftButtonPressEvent"
iren SetEventInformationFlipY 100 100 0 0 0 0 0
iren InvokeEvent "MouseMoveEvent"
iren InvokeEvent "LeftButtonReleaseEvent"
