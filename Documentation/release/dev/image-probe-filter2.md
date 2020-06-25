## Probe filter for imaging pipeline

The `vtkImageProbeFilter` is similar to `vtkProbeFilter`, but it is
designed for the VTK imaging pipeline and requires that the Source data
is `vtkImageData`.  It utilizes the `vtkImageInterpolator` classes (also
used by `vtkImageReslice`), rather than using the geometry pipeline's
cell and point interpolators.  It is an SMP filter, and can therefore
be accelerated with TBB.
