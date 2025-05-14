## Add support for oriented images in vtkProbeFilter

vtkProbeFilter has been improved to support oriented images.
The implementation now rely on the native support of orientation
in the API of vtkImageData.
