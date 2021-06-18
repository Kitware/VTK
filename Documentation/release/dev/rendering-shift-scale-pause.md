## Shift+scale rendering parameters

VTK's OpenGL polydata mappers provide a way to deal with jitter
caused by rendering single-precision vertex coordinates far from
their origin – they translate (shift) and scale the coordinates
sent to the vertex-buffer object (VBO). For large data, this
approach was causing issues as re-uploading a new VBO with a
different shift+scale would happen while users were interacting
with the camera. The VBO upload would take long enough the
framerate would drop, causing a visual "stutter."

To deal with this issue, polydata mappers include a new
member variable named _PauseShiftScale_ that can be used
to suspend updates to VBOs during user interactions.
You are responsible for setting this variable as needed,
but the intended use is to add observers to relevant render-window
interactors and set PauseShiftScale to true when a
`vtkCommand::StartInteractionEvent` occurs, set it to false
some time after `vtkCommand::EndInteractionEvent`, and then
force a render (causing any shift+scale parameters to be
recomputed and VBOs to be re-uploaded) after the user has
stopped changing the camera.
