## New USD exporter added

`vtkUSDExporter` is a new class that exports VTK scenes to Universal Scene Description (USD) files (.usd, .usda (ASCII), and .usdc (binary)). It is part of a new `vtk::IOUSD` module that depends on [OpenUSD](https://github.com/PixarAnimationStudios/OpenUSD) to write USD files

The `vtkUSDExporter` class supports exporting the following scene elements:
* Surface polygonal geometry (verts and lines are not supported in
  USD) including point normals if available, cell normals otherwise.
* Color mapped surfaces through export of color map textures and
  texture coordinates on surfaces.
* Directional lights.
* The current camera, including position and direction. Field of
  view is hard-coded for now.
* Actor transforms are exported.
* Solid colored surfaces are exported, including physically-based
  rendering (PBR) properties supported by the USD format when PBR is
  enabled.
* 3D widgets that are subclasses of `vtkWidgetRepresentation` are
  not exported.
* All surfaces from composite poly data are exported.
