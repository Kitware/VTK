## GPU volume ray cast reads the correct depth with multiple viewports

`vtkGPUVolumeRayCastMapper` no longer carves a volume with opaque geometry from
another viewport when several renderers share one render window with depth
peeling for volumes enabled. When the volume was rendered inside a depth peeling
pass, the mapper captured the opaque depth from the window's lower-left tile
instead of the renderer's own viewport, so a volume in an off-origin viewport
was clipped by that tile's geometry. The mapper now reads the depth from each
renderer's own tiled origin.
