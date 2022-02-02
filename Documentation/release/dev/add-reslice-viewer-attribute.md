# vtkResliceImageViewer

Add the possibility to apply a factor when scrolling.
Applying a non integer value in the factor induce a round to
calculate the correct slice when we are in Axis aligned mode.
Otherwise, in oblique mode, nothing else is applied. Only the factor to
the increment used to compute the new plane position.
