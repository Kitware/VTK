## Support image direction in vtkImageReslice

The vtkImageReslice filter now supports oriented images, and can reslice
an image into a new orientation via the new SetOutputDirection() method.
