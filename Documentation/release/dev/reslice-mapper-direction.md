## Support image direction in vtkImageResliceMapper

The vtkImageResliceMapper class correctly displays oriented images, just
as the vtkImageSliceMapper class does.  This allows the display of
arbitrary oblique slices of oriented images, including those where the
orientation matrix has a negative determinant.
