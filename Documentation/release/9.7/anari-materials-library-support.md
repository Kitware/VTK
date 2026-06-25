## ANARI material library support

VTK now offers material library support for ANARI rendering through a shared
`vtkRenderMaterialLibrary` base class in RenderingCore. This replaces
renderer-specific implementations with a unified framework that works across
ANARI, OSPRay, and future backends. The two backends currently implemented are
`vtkOSPRayMaterialLibrary` and `vtkANARIMaterialLibrary`.

Texture I/O is now handled with a common `ReadTextureFileOrData`
interface, to allow each backend to do its parameter validation and serialization.

The ANARI implementation includes case-insensitive parameter lookups and a
`MATERIAL_NAMES` macro for runtime introspection, making it easier to work with
materials programmatically.

Here's how you define materials for ANARI in a material file:

```mtl
newmtl glossy_red
ka 0.1 0.1 0.1
kd 0.8 0.0 0.0
ks 0.9 0.9 0.9
ns 128.0
map_kd red_texture.png

newmtl matte_blue
ka 0.05 0.05 0.05
kd 0.0 0.0 0.8
ks 0.1 0.1 0.1
ns 8.0
```

Then load and use the material library with the ANARI renderer:

```python
from vtkmodules.vtkRenderingAnari import vtkANARIMaterialLibrary, vtkAnariPass, vtkAnariSceneGraph
from vtkmodules.vtkRenderingCore import vtkRenderer

material_lib = vtkANARIMaterialLibrary()
material_lib.ImportMaterials("materials.mtl")

vtkAnariSceneGraph.SetMaterialLibrary(material_lib, renderer)

renderer = vtkRenderer()
anari_pass = vtkAnariPass()
renderer.SetPass(anari_pass)
```

Or in C++:

```cpp
auto material_lib = vtkNew<vtkANARIMaterialLibrary>();
material_lib->ImportMaterials("materials.mtl");

vtkAnariSceneGraph::SetMaterialLibrary(material_lib, renderer);

auto renderer = vtkNew<vtkRenderer>();
auto anari_pass = vtkNew<vtkAnariPass>();
renderer->SetPass(anari_pass);
```
