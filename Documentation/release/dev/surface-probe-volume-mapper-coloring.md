## Add support for coloring with LUT in vtkOpenGLSurfaceProbeVolumeMapper

Allows vtkOpenGLSurfaceProbeVolumeMapper to use a lookup table for coloring.
If a lookup table is provided, a texture map is used for coloring in order to map 1-component image through the lookup table when probing.
The window/level values are used to rescale scalar values before mapping unless `UseLookupTableScalarRange` is enabled, in which case the table range will be used.
If no lookup table is specified, scalar values are mapped directly with window/level values.
Also add support for multi-components source images.
Images with 3 or 4 components are considered as RGB or RGBA images and the probed pixels are used to color the output directly.
