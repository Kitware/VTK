## Fix interpolate scalars before mapping in surface LIC mappers

A scalar mapping bug has been fixed in the surface LIC mappers `vtkBatchedSurfaceLICMapper` and `vtkSurfaceLICMapper`. The color values were being interpolated after mapping the scalars to the colors, even when the `InterpolateScalarsBeforeMapping` flag was set to true. Now, interpolation occurs before mapping the scalars to colors when the flag is enabled.
