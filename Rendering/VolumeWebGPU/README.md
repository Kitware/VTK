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
| `SyncDeviceResources` | Tabulate the transfer functions, classify the volume into cells, and offer one contribution per connected port to the coordinator of the renderer. |
| `RecordingCommands` | Hand the volume to `vtkWebGPURenderer::AddPostRasterizationProp`. |
| `RenderPostRasterization` | Whichever volume arrives first has the coordinator decide which bricks to keep resident, write the uniforms, build the bind group around the finished depth buffer, and record one ray casting pass for all of them. The rest find the work already done. |

The mapper keeps what belongs to one input: the tabulated transfer functions,
the cell classification, and the ability to hand over the voxels of any one
brick. The coordinator keeps what the pass shares: the brick pool, the page
table, the transfer function texture arrays, the cell buffer, the parameter
buffers, the bind group and the pipeline. A contribution
(`Private/vtkWebGPUVolumeRayCastContribution.h`) is how one crosses to the
other, and carries a time stamp so that layers and cells that did not change
are not uploaded again.

Notice which way the voxels travel. The mapper never uploads a volume; it
answers for one brick when asked. Deciding what is worth having on the device
needs the camera and the memory budget, which the coordinator has and the mapper
does not, so residency is the coordinator's to decide and fetching is the
mapper's to answer.

Settings that describe the march rather than a single volume, the blend mode
above all, are handled specially. When volumes do not overlap (based on their
bounding boxes), each can use its own blend mode. When volumes overlap, they
must all use the same blend mode - an error is reported if they don't, since
overlapping volumes with different blend modes cannot be composited correctly.
Everything that is genuinely per volume, including clipping planes, transfer
functions, and shading parameters, lives in the per volume parameters.

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

## Bricks, and how a volume reaches the device

No volume is bound as a texture of its own. Every volume is cut into bricks of
`vtkWebGPUVolumeRayCastCoordinator::PoolBrickSize` voxels a side, the bricks that
are resident live in one shared pool texture
(`Private/vtkWebGPUVolumeBrickPool.h`), and a page table says which pool slot
holds which brick. There are two pools, one for eight bit scalars and one for
floats, so how many volumes a pass can hold is a question about the parameter
buffer rather than about texture bindings, and how much data they hold is a
question about the pool budget rather than about any single texture.

Each slot carries a one voxel apron around its brick, taken from the brick's
neighbours. That is what makes the cutting invisible: the eight texels the
filter reaches for always land inside a single slot, so one page lookup serves
the whole interpolation and a brick boundary reads exactly what a single large
texture would have read. It is also what lets the pools be filtered in hardware
rather than interpolated by hand - VTK asks its WebGPU device for
Float32Filterable, so every format a pool uses can be sampled directly.

The pool holds `BrickPoolSizeMB` megabytes at most, and grows only as far as the
volumes in the scene actually need. When they need more than that, the bricks
nearest the camera and inside the view frustum are the ones kept: bricks are
sorted by distance, what is already resident is claimed before anything is
evicted, and slots are recycled least recently used first, never taking one the
frame being drawn has already claimed. A brick that is not resident is stepped
over rather than read, and `MaximumBricksPerFrame` caps how many arrive per
frame, so a volume larger than the pool draws immediately and fills in over the
frames that follow. `GetComplete()` says whether the last frame got everything
it asked for.

## Streaming, and never reading a volume whole

Residency decides what is kept on the device out of a volume that has been read.
Streaming asks the question one stage earlier: what is worth reading at all.

When the source of an input publishes
`vtkStreamingDemandDrivenPipeline::CAN_PRODUCE_SUB_EXTENT()` and the volume is
larger than the mapper's `StreamingThresholdMB`, the input is never updated
whole. `vtkVolume::RenderVolumetricGeometry` calls `Mapper->Update()` before
every render, so the mapper overrides `ProcessRequest` and answers
`REQUEST_UPDATE_EXTENT` for a streamed port with an empty extent, which asks the
pipeline for nothing. Bricks are then fetched one at a time as the coordinator
claims them, each by an `Update(port, requests)` over that brick's extent grown
by its apron.

What that costs is knowing the volume before reading it. The scalar range, which
the transfer functions are tabulated over, has to come from the source's
`vtkDataObject::FIELD_RANGE()` rather than from the array; without one the
transfer functions' own range is used and a warning is issued. Cell
classification cannot be done up front either, so cells start out unknown and
are classified as the bricks covering them arrive. An unknown cell is treated as
full - opacity 1, varying - so the march is conservative until it knows better,
and the frame after a brick lands is the one that sees its cells. That is why
`GetComplete()` stays false for a frame after the last brick.

An input with a mask, a second transfer function axis array, cell scalars or
ghost arrays is read whole however large it is: the first two are whole volumes
of their own, and the last two are defined against a whole grid. A mask and a
second axis show in the pipeline's metadata; cell and ghost arrays do not, so a
single voxel is read first and what that probe finds decides whether the input
is streamed at all. A refusal lasts until something upstream of the input
changes, at which point it is asked again.

## Interpolation, and the finished colour

`vtkVolumeProperty::InterpolationType` picks between reading the nearest voxel
and interpolating between the eight around it. Nearest does not need a sampler
of its own: landing the sample exactly on a texel centre puts all of the
filter's weight on one texel, which is the same thing for less. Note that
vtkVolumeProperty starts at nearest, not at linear.

`vtkGPUVolumeRayCastMapper::FinalColorWindow` and `FinalColorLevel` put a window
and level over the finished colour, as a scale of one over the window and a bias
of a half less the level over the window. The colour is premultiplied by then,
so the bias is weighed by the alpha it was multiplied through by. Like the blend
mode these describe the pass rather than one volume, so the primary input
settles them.

## Finding empty space

Where a volume is empty is a finer question than what is worth keeping on the
device, so it is asked at a different scale. Each input is measured in cells of
the mapper's `BrickSize` voxels, with a voxel of overlap so a cell also covers
what interpolation produces inside it. Whenever the transfer functions change,
every cell is classified into the largest opacity the functions map it to and
how much it varies, using a sparse range table over the opacity table so the
classification costs one lookup per cell rather than a scan. The shader reads
that classification to jump over cells that map to nothing and to choose a step
between `MinimumSampleDistanceFactor` and `MaximumSampleDistanceFactor` times
the base distance. The coordinator reads it too: a brick is left out of the pool
only when every cell in it is empty.

Opacity is corrected for the distance a step actually travels, so a cell of
uniform material composites to the same colour however coarsely it is crossed.
The intensity blend modes weigh every sample equally and only mean what they say
when the samples are evenly spaced, so adaptive sampling is bypassed for them -
and so is dropping empty bricks, because those modes look at the scalars rather
than at what the transfer functions make of them.

## Limits

- Twelve volumes per renderer per pass, set by how many the shared parameter
  buffer describes. Anything beyond that is not drawn, with a warning. This is a
  limit on how many volumes, not on how large they are.
- Six lights per renderer, six clipping planes per volume and sixteen iso
  values. These are the lengths of fixed arrays in the shader's uniform
  buffers, and of the arrays a ray keeps while it marches. A scene that asks for
  more gets the first ones, with a warning. Raising a limit means changing the
  constant in `wgsl/VolumeRayCast.wgsl` and its twin in
  `vtkWebGPUVolumeRayCastCoordinatorInternals.h`.
- Rectilinear grids, volumetric scattering and reduced image sample distances are
  not implemented.
