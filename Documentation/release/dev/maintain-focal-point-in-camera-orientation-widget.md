## Maintaining Focal Point Consistency in `vtkCameraOrientationWidget`

The `vtkCameraOrientationWidget` previously called `ResetCamera()` during animation frames and selection actions, which would recalculate the focal point.

This behavior is now controlled by the `ShouldResetCamera` flag. The default value is `True` to preserve existing behavior, but you can set it to `False` to prevent the camera from resetting its focal point during interactions.
