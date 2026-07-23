## vtkTrackballRotate: fix rotation about a non-default center

`vtkTrackballRotate` now correctly rotates the camera about the center of
rotation set on `vtkInteractorStyleManipulator`. Previously the rotation
transform was missing the translation to the center of rotation, so the
camera rotated about the world origin and drifted away from the scene on
every mouse move whenever a non-zero center of rotation was set.
