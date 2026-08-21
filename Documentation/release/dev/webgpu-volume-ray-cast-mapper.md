## WebGPU volume rendering

VTK now renders volumes on the WebGPU backend. `vtkWebGPUVolumeRayCastMapper`
lives in the new `VTK::RenderingVolumeWebGPU` module and overrides
`vtkGPUVolumeRayCastMapper`, so the same code that renders a volume with OpenGL
renders it with WebGPU once you ask for that backend, for instance by passing
`--vtk-factory-prefer=RenderingBackend=WebGPU`.

It composites, maximum and minimum intensity, average intensity and additive
blending, colour, scalar opacity and gradient opacity transfer functions,
Blinn-Phong shading from the lights of the renderer, clipping planes, and it
terminates rays against the opaque geometry already in the frame.

Three things work differently from `vtkOpenGLGPUVolumeRayCastMapper`.

Volumes that share a renderer are ray cast together and may overlap in space.
Adding the props is all it takes:

```cpp
renderer->AddVolume(ct);   // its own mapper and transfer functions
renderer->AddVolume(pet);  // its own mapper and transfer functions
```

There is nothing to assemble by hand: no `vtkMultiVolume`, and no second input
port on one mapper. Where two volumes occupy the same space they are blended as
media that share the sample, so the picture does not depend on the order the
props were added in. A single mapper may still carry several inputs on
consecutive ports, each with its own `vtkVolumeProperty` set through
`SetVolumeProperty()`, and those join the same pass; a `vtkMultiVolume` still
works too. Up to twelve volumes are ray cast in one pass.

`vtkWebGPUVolumeRayCastCoordinator` is where this happens. There is one per
renderer, created on demand, and it is not something you normally touch.

The distance between samples is not fixed along a ray. The mapper keeps a
coarse min/max structure over each input and combines it with the current
transfer functions to jump over regions those functions map to nothing and to
sample regions that carry detail more densely than regions that do not. Turn
this off with `SetAdaptiveSampling(false)` to march with the same distance
everywhere; `SetMinimumSampleDistanceFactor` and
`SetMaximumSampleDistanceFactor` bound how far the distance is allowed to move.

Volumes are not uploaded whole. Every volume is cut into bricks, and the bricks
the rays actually reach are kept in a pool shared by the whole renderer, with a
page table saying which pool slot holds which brick. Each brick carries a one
voxel apron taken from its neighbours, so the cutting is invisible: a ray
crossing from one brick to the next reads exactly what it would have read from a
single large texture.

That makes a volume larger than device memory drawable. When the volumes in a
scene need more than `vtkWebGPUVolumeRayCastCoordinator::SetBrickPoolSizeMB()`
allows, the bricks nearest the camera and inside the view frustum are the ones
kept, and the rest arrive over the frames that follow, capped by
`SetMaximumBricksPerFrame()`. `GetComplete()` says whether the last frame got
everything it asked for. A pass that fits in the pool - which is the ordinary
case - is complete in one frame and none of this is visible.

The `VTK::RenderingVolume` C++ tests now run once per rendering backend, the way
the `VTK::RenderingCore` ones do, so their names carry a backend prefix:
`VTK::RenderingVolumeCxx-TestGPURayCastAdditive` is now
`VTK::RenderingVolumeCxx-OpenGL-TestGPURayCastAdditive` and
`VTK::RenderingVolumeCxx-WebGPU-TestGPURayCastAdditive`. The
`VTK::RenderingVolumeOpenGL2` tests that only used the mappers through their base
classes moved to `VTK::RenderingVolume` to be run the same way; the ones that
reach for depth peeling, user shaders, screen space ambient occlusion, texture
streaming or the surface probe mapper stayed behind, as did the tests of the
fixed point, tetrahedra and unstructured grid mappers, which only OpenGL
implements.
