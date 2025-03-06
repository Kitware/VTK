# VTK::RenderingCellGrid

This module adds support for rendering and hardware-selection of vtkCellGrid data.
Currently, surface (but not volume) rendering is supported; this means
that surfaces of cells may be rendered and they may be colored and
semi-transparent, but that functions defined over volumetric cells
will not affect the attenuation, coloring, or scattering of light
interior to the cell.

The rendering technique uses the VTK::RenderingOpenGL2 module's
[vtkDrawTexturedElements](https://vtk.org/doc/nightly/html/classvtkDrawTexturedElements.html)
class to upload array data as textures and use it to determine the
location, tessellation subdivision, and color of triangle vertices and
their resulting fragments.
