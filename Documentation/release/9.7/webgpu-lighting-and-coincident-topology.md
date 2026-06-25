## WebGPU Lighting and Coincident Topology Support

VTK's WebGPU rendering backend now supports the full set of VTK light types and
resolves z-fighting on coplanar geometry via coincident topology depth offsets.

### Full Light Type Support

The WebGPU rendering backend handles all VTK light types - *Headlight*,
*CameraLight*, *SceneLight* as well as positional and directional lights.

![Stanford dragon rendered with vtkLightKit (key, fill, back, and head lights
active)](http://vtk.org/files/ExternalData/SHA512/daf22135fad38f2583e1bfbfa803b562bac8b649e1fb6abebff3cacc7af64bb31644bbcc239c4597495094569d11e020e926122f6dfe799e2bc15d89496da8d3)
*Stanford dragon rendered with `vtkLightKit`, showing different light types
(camera and headlight) with the WebGPU backend.*

### Coincident Topology Depth Offsets

The WebGPU rendering backend now resolves z-fighting between coplanar geometry
(e.g., surface meshes with overlaid wireframe or point representations) using
the coincident topology resolution API.

![Coplanar surface, wireframe, and point representations rendered without
z-fighting](http://vtk.org/files/ExternalData/SHA512/3c5ff1e204734cc1afdcb7613baf32cb207c0ad501254479b507b7fa68d9bbfd7465bedcdce2e162db9d569b533394fa1286b243749f5ee804035652b1cdfee5)
*Coplanar surface, wireframe, and point representations resolved cleanly using
coincident topology depth offsets with the WebGPU backend.*
