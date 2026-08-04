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

Two things work differently from `vtkOpenGLGPUVolumeRayCastMapper`.

You can hand the mapper several inputs and let them overlap in space:

```cpp
mapper->SetInputConnection(0, ct->GetOutputPort());
mapper->SetInputConnection(1, pet->GetOutputPort());
mapper->SetVolumeProperty(0, ctProperty);
mapper->SetVolumeProperty(1, petProperty);
```

No `vtkMultiVolume` is needed to hold them together, and the inputs may occupy
the same space. Where they do, the mapper blends them as media that share the
sample, so the picture does not depend on the order the inputs were connected
in. Up to twelve inputs are ray cast in one pass. A `vtkMultiVolume` is still
accepted for source compatibility.

The distance between samples is not fixed along a ray. The mapper keeps a
coarse min/max structure over each input and combines it with the current
transfer functions to jump over regions those functions map to nothing and to
sample regions that carry detail more densely than regions that do not. Turn
this off with `SetAdaptiveSampling(false)` to march with the same distance
everywhere; `SetMinimumSampleDistanceFactor` and
`SetMaximumSampleDistanceFactor` bound how far the distance is allowed to move.

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
