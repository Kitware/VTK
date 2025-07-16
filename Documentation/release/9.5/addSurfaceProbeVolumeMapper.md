# Add SurfaceProbeVolumeMapper

vtkOpenGLSurfaceProbeVolumeMapper is a PolyDataMapper colored with probed volume data.
The mapper accepts three inputs: the Input, the Source and an optional ProbeInput.
The Source data defines the vtkImageData from which scalar values are interpolated.
The Input data defines the rendered surface.
The ProbeInput defines the geometry used to interpolate the source data.
If the ProbeInput is not specified, the Input is used both for probing and rendering.
Projecting the scalar values from the ProbeInput to the Input is done thanks to texture
coordinates. Both inputs must provide texture coordinates in the [0, 1] range.
