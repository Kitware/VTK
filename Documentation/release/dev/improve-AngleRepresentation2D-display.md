## Improve vtkAngleRepresentation2D rendering behavior when interacting

`vtkAngleRepresentation2D` is computing the coordinate of the arc representing the angle in screen space,
which could result in a weird behavior when interacting with the view and changing the camera position. The class
now provides a new API `SetForce3DArcPlacement` that allows you to force correct 3D placement of the arc so that
placement always looks good.
