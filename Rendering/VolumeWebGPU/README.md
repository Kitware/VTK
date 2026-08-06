# VTK::RenderingVolumeWebGPU

Volume rendering for the WebGPU backend. Two classes:

- `vtkWebGPUVolumeRayCastMapper`, an object factory override of
  `vtkGPUVolumeRayCastMapper` tagged with `RenderingBackend=WebGPU` so that
  `vtkObjectFactory` preferences pick between it and the OpenGL mapper. It
  prepares inputs; it does not draw.
- `vtkWebGPUVolumeRayCastCoordinator`, one per renderer, which gathers the
  inputs of every volume in that renderer and marches them together.

## Why a coordinator

Volume rendering only composites correctly when the volumes that share a ray are
sampled together. If each mapper drew its own volume in turn, each would
composite a finished image over the last, which is wrong wherever two volumes
occupy the same space and order dependent everywhere else. That is the reason
the OpenGL backend needs a `vtkMultiVolume` and refuses to let its inputs
overlap.

Here the volumes are brought back together at the renderer instead. Adding
several `vtkVolume` props is enough, each with its own mapper and its own
transfer functions, overlapping or not. Several inputs on one mapper still work
and join the same pass, and so does a `vtkMultiVolume`.

## How a frame is put together

The mapper does its work across three of the stages `vtkWebGPURenderer` runs
through.

| Stage | What happens |
| --- | --- |
| `SyncDeviceResources` | Upload the volume textures, tabulate the transfer functions, classify the blocks, and offer one contribution per connected port to the coordinator of the renderer. |
| `RecordingCommands` | Hand the volume to `vtkWebGPURenderer::AddPostRasterizationProp`. |
| `RenderPostRasterization` | Whichever volume arrives first has the coordinator write the uniforms, build the bind group around the finished depth buffer, and record one ray casting pass for all of them. The rest find the work already done. |

The mapper keeps what belongs to one input: its 3D texture, the tabulated
transfer functions and the block classification. The coordinator keeps what the
pass shares: the transfer function texture arrays, the block buffer, the
parameter buffers, the bind group and the pipeline. A contribution
(`Private/vtkWebGPUVolumeRayCastContribution.h`) is how one crosses to the
other, and carries a time stamp so that layers and blocks that did not change
are not uploaded again.

Settings that describe the march rather than a single volume, the blend mode
above all, are taken from the mapper of the first input, with a warning if the
others disagree; a pass can only be marched one way. Everything that is genuinely
per volume, including clipping planes, lives in the per volume parameters.

Nothing is drawn in the main render pass. That pass owns the depth attachment,
and the ray march has to read the depth the opaque geometry left behind in order
to stop rays at surfaces, which a pass cannot do with an attachment it is still
writing to. Waiting until the main pass has been flushed also means the volumes
are composited over a finished frame.

## The shader

`wgsl/VolumeRayCast.wgsl` holds both stages.

The vertex stage draws the world space box that bounds every input, keeping the
faces that point away from the camera so that one ray is cast per covered pixel
even when the camera is inside the box. When the near plane cuts into the box
the faces stop covering the viewport, and the mapper switches to a full screen
triangle instead.

The fragment stage rebuilds the ray by unprojecting the pixel at both ends of
the clip volume, which works for parallel and perspective cameras alike and
parameterises the ray by world distance from the near plane. That parameter is
shared by every input no matter how each one is placed, which is what lets
overlapping inputs be composited correctly: at each step the shader collects the
inputs that contain the current point, blends them as media that share it, and
composites the result once.

## Sampling

Each input is measured in blocks of `BrickSize` voxels, with a voxel of overlap
so a block also covers what interpolation produces inside it. Whenever the
transfer functions change, every block is classified into the largest opacity
the functions map it to and how much it varies, using a sparse range table over
the opacity table so the classification costs one lookup per block rather than a
scan. The shader reads that classification to jump over blocks that map to
nothing and to choose a step between `MinimumSampleDistanceFactor` and
`MaximumSampleDistanceFactor` times the base distance.

Opacity is corrected for the distance a step actually travels, so a block of
uniform material composites to the same colour however coarsely it is crossed.
The intensity blend modes weigh every sample equally and only mean what they say
when the samples are evenly spaced, so adaptive sampling is bypassed for them.

## Limits

- Twelve volumes per renderer per pass, set by how many sampled textures a bind
  group may hold for one shader stage. Anything beyond that is not drawn, with a
  warning.
- Six lights and six clipping planes.
- The first component of the scalars is rendered; independent multi-component
  volumes and two dimensional transfer functions are not implemented.
- Cropping regions and AMR volumes are not implemented.
