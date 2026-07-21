# vtkCameraOrientationWidget supports rotating about CenterOfRotation

The `vtkCameraOrientationWidget` now respects the center of rotation provided by
the associated interactor style. A new `UseCenterOfRotation` property (default
`false`) controls this behavior:

- When `UseCenterOfRotation` is disabled (legacy default), the widget rotates the
  camera about its focal point.
- When `UseCenterOfRotation` is enabled, the widget rotates the camera about the
  center of rotation provided by the interactor style (`vtkInteractorStyle::GetCenterOfRotation`).
