# Fix cubic interpolation for small image dimensions

The cubic interpolation mode of vtkImageInterpolator could give
incorrect results input images with y or z dimensions of 2 or 3,
or x dimensions of 2 or 3 when the image had multiple components.
This bug occurred for scale and translation transformations, but
not for rotation transformations (except for rotations by multiples
of 90 degrees).
