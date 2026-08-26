// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Ray casts any number of vtkImageData volumes in a single pass.
//
// Rays carry a world space parameter shared by every volume, so overlapping inputs composite
// correctly. No volume is bound on its own: each is cut into bricks, resident bricks share one
// pool texture, and a page table says which slot holds which. Bricks that are absent or empty
// are stepped over, so a volume larger than device memory, or one arriving in partitions, draws
// as a single volume.

const MAX_VOLUMES: u32 = 12u;
const MAX_LIGHTS: u32 = 6u;
const MAX_CLIP_PLANES: u32 = 6u;
const MAX_ISO_VALUES: u32 = 16u;
const MAX_COMPONENTS: u32 = 4u;

// Page.flags
const PAGE_RESIDENT: u32 = 1u;

// GlobalUniforms.flags
const FLAG_JITTER: u32 = 1u;
const FLAG_ADAPTIVE: u32 = 2u;
const FLAG_SKIP_EMPTY: u32 = 4u;
const FLAG_FULL_SCREEN: u32 = 8u;
const FLAG_HAS_DEPTH: u32 = 16u;
const FLAG_DEFAULT_LIGHTING: u32 = 32u;
const FLAG_TWO_SIDED_LIGHTING: u32 = 64u;

// VolumeParams.flags
const VOLUME_FLAG_SHADE: u32 = 1u;
const VOLUME_FLAG_GRADIENT_OPACITY: u32 = 2u;
const VOLUME_FLAG_CROPPING: u32 = 4u;
const VOLUME_FLAG_NORMAL_FROM_OPACITY: u32 = 8u;
const VOLUME_FLAG_INDEPENDENT_COMPONENTS: u32 = 16u;
const VOLUME_FLAG_MASKED: u32 = 32u;
const VOLUME_FLAG_MASK_HIDES_NEIGHBOURS: u32 = 64u;
const VOLUME_FLAG_CELL_DATA: u32 = 128u;

// GlobalUniforms.blend_mode, matching vtkVolumeMapper::BlendModes
const BLEND_COMPOSITE: u32 = 0u;
const BLEND_MAXIMUM_INTENSITY: u32 = 1u;
const BLEND_MINIMUM_INTENSITY: u32 = 2u;
const BLEND_AVERAGE_INTENSITY: u32 = 3u;
const BLEND_ADDITIVE: u32 = 4u;
const BLEND_ISOSURFACE: u32 = 5u;
const BLEND_SLICE: u32 = 6u;

const FAR_AWAY: f32 = 3.4e38;

// One light of the renderer, in world space. `direction` points from the light towards what it
// lights; the positional fields are ignored unless `positional` is set.
struct Light {
  ambient: vec3<f32>,
  cone_angle: f32,
  diffuse: vec3<f32>,
  exponent: f32,
  specular: vec3<f32>,
  positional: f32,
  direction: vec3<f32>,
  padding0: f32,
  position: vec3<f32>,
  padding1: f32,
  attenuation: vec3<f32>,
  padding2: f32,
}

struct GlobalUniforms {
  view_projection: mat4x4<f32>,
  inv_view_projection: mat4x4<f32>,
  // World space bounds of the union of all the volumes, used as proxy geometry.
  bbox_min: vec3<f32>,
  jitter_scale: f32,
  bbox_max: vec3<f32>,
  empty_threshold: f32,
  camera_position: vec3<f32>,
  num_volumes: u32,
  // origin x, origin y, width, height, in framebuffer pixels
  viewport: vec4<f32>,
  num_lights: u32,
  flags: u32,
  blend_mode: u32,
  max_steps: u32,
  min_step_factor: f32,
  max_step_factor: f32,
  // Window the average intensity mode restricts itself to, in data units.
  average_range_minimum: f32,
  average_range_maximum: f32,
  // Slots per axis in the brick pool, and how many voxels wide one slot is (brick plus apron).
  pool_slot_grid: vec3<f32>,
  pool_slot_size: f32,
  num_iso_values: u32,
  padding0: f32,
  padding1: f32,
  padding2: f32,
  // Iso values of the primary volume, in data units and in order. Only x of each entry carries
  // a value; WGSL pads an array of scalars out to this anyway.
  iso_values: array<vec4<f32>, MAX_ISO_VALUES>,
  lights: array<Light, MAX_LIGHTS>,
}

struct VolumeParams {
  // texture space [0,1]^3 to world space, and back
  volume_matrix: mat4x4<f32>,
  inv_volume_matrix: mat4x4<f32>,
  dims: vec3<f32>,
  base_step: f32,
  spacing: vec3<f32>,
  brick_size: f32,
  page_dims: vec3<f32>,
  cell_size: f32,
  cell_dims: vec3<f32>,
  opacity_unit_distance: f32,
  // The cropping planes, in texture coordinates.
  cropping_lower: vec3<f32>,
  ambient: f32,
  cropping_upper: vec3<f32>,
  diffuse: f32,
  // Anything a volume can have one of per component is a four vector, because vtkVolumeProperty
  // carries up to four sets of transfer functions and the shader picks between them.
  value_scale: vec4<f32>,
  value_bias: vec4<f32>,
  scalar_range_min: vec4<f32>,
  scalar_range_scale: vec4<f32>,
  gradient_opacity_min: vec4<f32>,
  gradient_opacity_scale: vec4<f32>,
  component_weights: vec4<f32>,
  // Plane the slice blend mode cuts along, in world space.
  slice_plane: vec4<f32>,
  // Where this volume's pages and acceleration cells start in the shared tables.
  page_offset: u32,
  cell_offset: u32,
  // One bit per cropping region, in the order vtkVolumeMapper numbers them.
  cropping_flags: u32,
  flags: u32,
  // Clipping planes belong to the mapper that owns this input, so every input carries its own.
  clip_plane_count: u32,
  // Which pool this volume's bricks live in, matching the POOL_ constants.
  pool_kind: u32,
  num_components: u32,
  specular: f32,
  specular_power: f32,
  // The component the intensity modes, the iso surfaces and the acceleration structure read.
  primary_component: u32,
  padding0: f32,
  padding1: f32,
  clip_planes: array<vec4<f32>, MAX_CLIP_PLANES>,
}

// VolumeParams.pool_kind
const POOL_BYTE: u32 = 0u;
const POOL_FLOAT: u32 = 1u;
const POOL_BYTE_RGBA: u32 = 2u;
const POOL_FLOAT_RGBA: u32 = 3u;

// One entry per brick of every volume. Bricks that are resident name the pool slot they sit in;
// the rest are stepped over until they arrive.
struct Page {
  slot: u32,
  flags: u32,
}

@group(0) @binding(0) var<uniform> globals: GlobalUniforms;
@group(0) @binding(1) var<uniform> volumes: array<VolumeParams, MAX_VOLUMES>;
@group(0) @binding(2) var<storage, read> pages: array<Page>;
@group(0) @binding(3) var color_transfer_functions: texture_2d_array<f32>;
@group(0) @binding(4) var opacity_transfer_functions: texture_2d_array<f32>;
@group(0) @binding(5) var gradient_transfer_functions: texture_2d_array<f32>;
@group(0) @binding(6) var transfer_function_sampler: sampler;
@group(0) @binding(7) var opaque_depth: texture_depth_2d;
@group(0) @binding(8) var pool_byte: texture_3d<f32>;
@group(0) @binding(9) var pool_float: texture_3d<f32>;
@group(0) @binding(11) var pool_byte_rgba: texture_3d<f32>;
@group(0) @binding(12) var pool_float_rgba: texture_3d<f32>;
// One byte per voxel, non zero where the voxel is not to be drawn.
@group(0) @binding(13) var pool_mask: texture_3d<f32>;
// Per acceleration cell: x is the largest opacity the transfer functions map it to, y how much it
// varies, in [0,1]. Cells are finer than pool bricks: emptiness is a finer question than residency.
@group(0) @binding(10) var<storage, read> cells: array<vec2<f32>>;

//------------------------------------------------------------------------------
// Vertex stage
//------------------------------------------------------------------------------

@vertex
fn vertexMain(@builtin(vertex_index) vertex_id: u32) -> @builtin(position) vec4<f32> {
  if ((globals.flags & FLAG_FULL_SCREEN) != 0u) {
    // A triangle that covers the whole clip volume, used when the near plane cuts into the
    // proxy box and its back faces can no longer be relied on to cover every pixel. Wound the
    // same way round as the faces of the box that survive culling.
    var corners = array<vec2<f32>, 3>(
      vec2<f32>(-1.0, -3.0), vec2<f32>(3.0, 1.0), vec2<f32>(-1.0, 1.0));
    return vec4<f32>(corners[vertex_id], 0.0, 1.0);
  }

  // Unit cube corners wound counter clockwise as seen from outside. The framebuffer flips y, so
  // those faces read as back facing and are the ones culling keeps.
  var cube = array<vec3<f32>, 36>(
    vec3<f32>(0, 0, 0), vec3<f32>(0, 1, 0), vec3<f32>(1, 1, 0),
    vec3<f32>(0, 0, 0), vec3<f32>(1, 1, 0), vec3<f32>(1, 0, 0),
    vec3<f32>(0, 0, 1), vec3<f32>(1, 0, 1), vec3<f32>(1, 1, 1),
    vec3<f32>(0, 0, 1), vec3<f32>(1, 1, 1), vec3<f32>(0, 1, 1),
    vec3<f32>(0, 0, 0), vec3<f32>(0, 0, 1), vec3<f32>(0, 1, 1),
    vec3<f32>(0, 0, 0), vec3<f32>(0, 1, 1), vec3<f32>(0, 1, 0),
    vec3<f32>(1, 0, 0), vec3<f32>(1, 1, 0), vec3<f32>(1, 1, 1),
    vec3<f32>(1, 0, 0), vec3<f32>(1, 1, 1), vec3<f32>(1, 0, 1),
    vec3<f32>(0, 0, 0), vec3<f32>(1, 0, 0), vec3<f32>(1, 0, 1),
    vec3<f32>(0, 0, 0), vec3<f32>(1, 0, 1), vec3<f32>(0, 0, 1),
    vec3<f32>(0, 1, 0), vec3<f32>(0, 1, 1), vec3<f32>(1, 1, 1),
    vec3<f32>(0, 1, 0), vec3<f32>(1, 1, 1), vec3<f32>(1, 1, 0));

  let world = mix(globals.bbox_min, globals.bbox_max, cube[vertex_id]);
  return globals.view_projection * vec4<f32>(world, 1.0);
}

//------------------------------------------------------------------------------
// Volume sampling
//------------------------------------------------------------------------------

// Where a texture coordinate lands, in samples. Point samples sit on the volume edges so the
// texture runs centre to centre; cell samples sit half a cell in and the outer half cell repeats.
fn voxelOf(volume_index: u32, tex_coord: vec3<f32>) -> vec3<f32> {
  let params = volumes[volume_index];
  let clamped = clamp(tex_coord, vec3<f32>(0.0), vec3<f32>(1.0));
  if ((params.flags & VOLUME_FLAG_CELL_DATA) != 0u) {
    return clamp(clamped * params.dims - vec3<f32>(0.5), vec3<f32>(0.0),
                 max(params.dims - vec3<f32>(1.0), vec3<f32>(0.0)));
  }
  return clamped * max(params.dims - vec3<f32>(1.0), vec3<f32>(1.0));
}

fn pageAt(volume_index: u32, brick: vec3<i32>) -> Page {
  let params = volumes[volume_index];
  let dims = vec3<i32>(params.page_dims);
  let clamped = clamp(brick, vec3<i32>(0), dims - vec3<i32>(1));
  return pages[params.page_offset +
    u32(clamped.z * dims.y * dims.x + clamped.y * dims.x + clamped.x)];
}

// Where in the pool the first voxel of a slot's apron sits.
fn slotOrigin(slot: u32) -> vec3<i32> {
  let grid = vec3<u32>(globals.pool_slot_grid);
  let cell = vec3<u32>(slot % grid.x, (slot / grid.x) % grid.y, slot / (grid.x * grid.y));
  return vec3<i32>(cell) * i32(globals.pool_slot_size);
}

fn loadPool(kind: u32, coord: vec3<i32>) -> vec4<f32> {
  switch kind {
    case 0u: { return vec4<f32>(textureLoad(pool_byte, coord, 0).r, 0.0, 0.0, 0.0); }
    case 2u: { return textureLoad(pool_byte_rgba, coord, 0); }
    case 3u: { return textureLoad(pool_float_rgba, coord, 0); }
    default: { return vec4<f32>(textureLoad(pool_float, coord, 0).r, 0.0, 0.0, 0.0); }
  }
}

// Trilinear interpolation by hand. The pool is bound as unfilterable so that 8 bit and 32 bit
// volumes can share one bind group layout.
//
// The eight taps of one sample always land inside a single slot: a brick's apron carries the
// voxels of its neighbours, so a sample anywhere in the brick reaches no further than the apron.
// One page lookup therefore serves the whole interpolation, and a brick boundary is not a seam.
fn sampleVector(volume_index: u32, tex_coord: vec3<f32>) -> vec4<f32> {
  let params = volumes[volume_index];
  let voxel = voxelOf(volume_index, tex_coord);

  let brick = vec3<i32>(floor(voxel / params.brick_size));
  let page = pageAt(volume_index, brick);
  if ((page.flags & PAGE_RESIDENT) == 0u) {
    return vec4<f32>(0.0);
  }

  // The apron takes up the first voxel of the slot, so the brick itself starts one in.
  let origin = slotOrigin(page.slot) + vec3<i32>(1);
  let local = voxel - vec3<f32>(brick) * params.brick_size;
  let base = floor(local);
  let frac = local - base;

  let i0 = origin + vec3<i32>(base);
  let i1 = i0 + vec3<i32>(1);
  let kind = params.pool_kind;

  let c000 = loadPool(kind, vec3<i32>(i0.x, i0.y, i0.z));
  let c100 = loadPool(kind, vec3<i32>(i1.x, i0.y, i0.z));
  let c010 = loadPool(kind, vec3<i32>(i0.x, i1.y, i0.z));
  let c110 = loadPool(kind, vec3<i32>(i1.x, i1.y, i0.z));
  let c001 = loadPool(kind, vec3<i32>(i0.x, i0.y, i1.z));
  let c101 = loadPool(kind, vec3<i32>(i1.x, i0.y, i1.z));
  let c011 = loadPool(kind, vec3<i32>(i0.x, i1.y, i1.z));
  let c111 = loadPool(kind, vec3<i32>(i1.x, i1.y, i1.z));

  let c00 = mix(c000, c100, frac.x);
  let c10 = mix(c010, c110, frac.x);
  let c01 = mix(c001, c101, frac.x);
  let c11 = mix(c011, c111, frac.x);
  let raw = mix(mix(c00, c10, frac.y), mix(c01, c11, frac.y), frac.z);

  return raw * params.value_scale + params.value_bias;
}

// Whether a sample reads voxels it is not allowed to. A binary mask cuts exactly where it says,
// so only the nearest voxel counts; ghosted and blanked voxels are not data at all, so anything
// the interpolation would draw them into goes with them.
fn masked(volume_index: u32, tex_coord: vec3<f32>) -> bool {
  let params = volumes[volume_index];
  if ((params.flags & VOLUME_FLAG_MASKED) == 0u) {
    return false;
  }
  let voxel = voxelOf(volume_index, tex_coord);

  let brick = vec3<i32>(floor(voxel / params.brick_size));
  let page = pageAt(volume_index, brick);
  if ((page.flags & PAGE_RESIDENT) == 0u) {
    return true;
  }
  let origin = slotOrigin(page.slot) + vec3<i32>(1);
  let local = voxel - vec3<f32>(brick) * params.brick_size;

  if ((params.flags & VOLUME_FLAG_MASK_HIDES_NEIGHBOURS) == 0u) {
    let nearest = origin + vec3<i32>(round(local));
    return textureLoad(pool_mask, nearest, 0).r > 0.0;
  }

  let i0 = origin + vec3<i32>(floor(local));
  let i1 = i0 + vec3<i32>(1);
  var hidden = 0.0;
  for (var corner = 0; corner < 8; corner++) {
    let coord = vec3<i32>(select(i0.x, i1.x, (corner & 1) != 0),
                          select(i0.y, i1.y, (corner & 2) != 0),
                          select(i0.z, i1.z, (corner & 4) != 0));
    hidden = max(hidden, textureLoad(pool_mask, coord, 0).r);
  }
  return hidden > 0.0;
}

// The component the intensity modes, the iso surfaces and the acceleration structure look at.
fn sampleScalar(volume_index: u32, tex_coord: vec3<f32>) -> f32 {
  return sampleVector(volume_index, tex_coord)[volumes[volume_index].primary_component];
}

// Which of the twenty seven cropping regions a point falls in, numbered the way
// vtkVolumeMapper numbers them, or zero when the volume is not cropped.
fn croppingRegion(volume_index: u32, tex_coord: vec3<f32>) -> u32 {
  let params = volumes[volume_index];
  var region = 1;
  var scale = 1;
  for (var axis = 0; axis < 3; axis++) {
    let position = tex_coord[axis];
    let lower = params.cropping_lower[axis];
    let upper = params.cropping_upper[axis];
    var coordinate = 3;
    if (position < lower) {
      coordinate = 1;
    } else if (position < upper) {
      coordinate = 2;
    }
    region += (coordinate - 1) * scale;
    scale *= 3;
  }
  return u32(region);
}

// Whether a point is in a region the mapper was told to leave out.
fn cropped(volume_index: u32, tex_coord: vec3<f32>) -> bool {
  let params = volumes[volume_index];
  if ((params.flags & VOLUME_FLAG_CROPPING) == 0u) {
    return false;
  }
  let region = croppingRegion(volume_index, tex_coord);
  return ((params.cropping_flags >> (region - 1u)) & 1u) == 0u;
}

// Where component `component` of volume `volume_index` keeps its tabulated transfer functions.
fn transferLayer(volume_index: u32, component: u32) -> i32 {
  return i32(volume_index * MAX_COMPONENTS + component);
}

// A component's value put into the [0,1] its own transfer functions were tabulated over.
fn normalizeComponent(volume_index: u32, component: u32, value: f32) -> f32 {
  let params = volumes[volume_index];
  return clamp(
    (value - params.scalar_range_min[component]) * params.scalar_range_scale[component], 0.0, 1.0);
}

fn opacityOf(volume_index: u32, component: u32, normalized: f32) -> f32 {
  return textureSampleLevel(opacity_transfer_functions, transfer_function_sampler,
    vec2<f32>(normalized, 0.5), transferLayer(volume_index, component), 0.0).r;
}

fn colorOf(volume_index: u32, component: u32, normalized: f32) -> vec3<f32> {
  return textureSampleLevel(color_transfer_functions, transfer_function_sampler,
    vec2<f32>(normalized, 0.5), transferLayer(volume_index, component), 0.0).rgb;
}

// Transfer functions give opacity over one unit distance, so correct for the step actually taken.
// The OpenGL mapper bakes this into its tables, so it lands before components blend, not after.
fn correctOpacity(opacity: f32, step: f32, unit_distance: f32) -> f32 {
  return 1.0 - pow(1.0 - min(opacity, 0.9999), step / unit_distance);
}

// What the gradient opacity function of one component makes of a gradient. One when the volume
// has no gradient opacity, so it can always be multiplied in.
fn gradientOpacityOf(volume_index: u32, component: u32, gradient: vec3<f32>,
                     spacing: vec3<f32>) -> f32 {
  let params = volumes[volume_index];
  if ((params.flags & VOLUME_FLAG_GRADIENT_OPACITY) == 0u) {
    return 1.0;
  }
  // vtkVolumeShaderComposer measures the gradient against the aspect of the voxel rather than
  // its size, then against a quarter of the data range, and that is what the table is indexed by.
  let average = (spacing.x + spacing.y + spacing.z) / 3.0;
  let aspect = spacing * 2.0 / average;
  let physical = length(gradient / aspect);
  let position = clamp((physical - params.gradient_opacity_min[component]) *
    params.gradient_opacity_scale[component], 0.0, 1.0);
  return textureSampleLevel(gradient_transfer_functions, transfer_function_sampler,
    vec2<f32>(position, 0.5), transferLayer(volume_index, component), 0.0).r;
}

// What the scalar opacity function makes of the point, before any gradient opacity is applied.
fn sampleOpacity(volume_index: u32, tex_coord: vec3<f32>) -> f32 {
  let component = volumes[volume_index].primary_component;
  let scalar = sampleVector(volume_index, tex_coord)[component];
  return opacityOf(volume_index, component, normalizeComponent(volume_index, component, scalar));
}

// Central differences of opacity rather than of the scalar, as
// vtkVolumeMapper::ComputeNormalFromOpacity asks: surfaces follow what the transfer function shows.
fn computeOpacityGradient(volume_index: u32, tex_coord: vec3<f32>) -> vec3<f32> {
  let params = volumes[volume_index];
  let step = 1.0 / select(max(params.dims - vec3<f32>(1.0), vec3<f32>(1.0)), params.dims,
    (params.flags & VOLUME_FLAG_CELL_DATA) != 0u);
  let xp = sampleOpacity(volume_index, tex_coord + vec3<f32>(step.x, 0.0, 0.0));
  let xm = sampleOpacity(volume_index, tex_coord - vec3<f32>(step.x, 0.0, 0.0));
  let yp = sampleOpacity(volume_index, tex_coord + vec3<f32>(0.0, step.y, 0.0));
  let ym = sampleOpacity(volume_index, tex_coord - vec3<f32>(0.0, step.y, 0.0));
  let zp = sampleOpacity(volume_index, tex_coord + vec3<f32>(0.0, 0.0, step.z));
  let zm = sampleOpacity(volume_index, tex_coord - vec3<f32>(0.0, 0.0, step.z));
  return vec3<f32>(xp - xm, yp - ym, zp - zm);
}

// Central differences, returned in texture space. Divide component wise by twice the spacing to
// get the gradient with respect to world distance.
fn computeGradient(volume_index: u32, tex_coord: vec3<f32>) -> vec3<f32> {
  let params = volumes[volume_index];
  let step = 1.0 / select(max(params.dims - vec3<f32>(1.0), vec3<f32>(1.0)), params.dims,
    (params.flags & VOLUME_FLAG_CELL_DATA) != 0u);
  let xp = sampleScalar(volume_index, tex_coord + vec3<f32>(step.x, 0.0, 0.0));
  let xm = sampleScalar(volume_index, tex_coord - vec3<f32>(step.x, 0.0, 0.0));
  let yp = sampleScalar(volume_index, tex_coord + vec3<f32>(0.0, step.y, 0.0));
  let ym = sampleScalar(volume_index, tex_coord - vec3<f32>(0.0, step.y, 0.0));
  let zp = sampleScalar(volume_index, tex_coord + vec3<f32>(0.0, 0.0, step.z));
  let zm = sampleScalar(volume_index, tex_coord - vec3<f32>(0.0, 0.0, step.z));
  return vec3<f32>(xp - xm, yp - ym, zp - zm);
}

// The page the given point falls in.
fn pageOf(volume_index: u32, tex_coord: vec3<f32>) -> Page {
  let params = volumes[volume_index];
  let voxel = voxelOf(volume_index, tex_coord);
  return pageAt(volume_index, vec3<i32>(floor(voxel / params.brick_size)));
}

// What the transfer functions make of the acceleration cell the given point falls in.
fn cellAt(volume_index: u32, tex_coord: vec3<f32>) -> vec2<f32> {
  let params = volumes[volume_index];
  let voxel = voxelOf(volume_index, tex_coord);
  let cell = clamp(floor(voxel / params.cell_size), vec3<f32>(0.0),
                   params.cell_dims - vec3<f32>(1.0));
  return cells[params.cell_offset +
    u32(cell.z * params.cell_dims.y * params.cell_dims.x +
        cell.y * params.cell_dims.x + cell.x)];
}

// Distance, along the ray, from the current point to where it leaves the block of \a size voxels
// a side that it is in. Used for both the pool's bricks and the finer acceleration cells.
fn blockExit(volume_index: u32, origin: vec3<f32>, direction: vec3<f32>, t: f32,
             size: f32) -> f32 {
  let params = volumes[volume_index];
  let extent = select(max(params.dims - vec3<f32>(1.0), vec3<f32>(1.0)), params.dims,
    (params.flags & VOLUME_FLAG_CELL_DATA) != 0u);
  let voxel = (origin + t * direction) * extent;
  let block = floor(voxel / size);

  let lower = block * size / extent;
  let upper = (block + vec3<f32>(1.0)) * size / extent;

  let inverse = 1.0 / direction;
  let t0 = (lower - origin) * inverse;
  let t1 = (upper - origin) * inverse;
  let t_max = max(t0, t1);
  return max(min(min(t_max.x, t_max.y), t_max.z) - t, 0.0);
}

//------------------------------------------------------------------------------
// Shading
//------------------------------------------------------------------------------

// Follows vtkVolumeShaderComposer, whose two formulations disagree on sign conventions, so both
// are written out. `normal` is the gradient, pointing into the volume; a light's `direction`
// points from the light. Two sided lighting, on by default, lets a surface be lit either way.
fn shade(volume_index: u32, color: vec3<f32>, tex_coord: vec3<f32>,
         gradient: vec3<f32>) -> vec3<f32> {
  let params = volumes[volume_index];
  let magnitude = length(gradient);
  if (magnitude < 1.0e-6) {
    // No direction to shade along. The ambient term still applies, as it does in OpenGL, where
    // a degenerate gradient becomes a zero normal rather than an unlit sample.
    return params.ambient * color;
  }

  let two_sided = (globals.flags & FLAG_TWO_SIDED_LIGHTING) != 0u;
  // computeGradient steps one voxel at a time, so it returns a difference per voxel. Scaling by
  // the voxel count restores the space the inverse transpose expects, for anisotropic volumes.
  let extent = max(params.dims - vec3<f32>(1.0), vec3<f32>(1.0));
  let normal =
    normalize((transpose(params.inv_volume_matrix) * vec4<f32>(gradient * extent, 0.0)).xyz);
  let world = (params.volume_matrix * vec4<f32>(tex_coord, 1.0)).xyz;
  // Towards the camera.
  let view = normalize(globals.camera_position - world);

  var lit: vec3<f32>;

  if ((globals.flags & FLAG_DEFAULT_LIGHTING) != 0u) {
    // One head light at full intensity. The light's own ambient colour is left out and the
    // ambient term is tinted with the voxel colour instead.
    let light = globals.lights[0];
    let to_light = -light.direction;
    var n_dot_l = dot(normal, to_light);
    let reflected = normalize(2.0 * n_dot_l * normal - to_light);
    let v_dot_r = dot(reflected, view);
    if (n_dot_l < 0.0 && two_sided) {
      n_dot_l = -n_dot_l;
    }
    var diffuse = vec3<f32>(0.0);
    var specular = vec3<f32>(0.0);
    if (n_dot_l > 0.0) {
      diffuse = n_dot_l * params.diffuse * light.diffuse * color;
      specular = pow(max(v_dot_r, 0.0), params.specular_power) * params.specular * light.specular;
    }
    lit = params.ambient * color + diffuse + specular;
  } else {
    var ambient_sum = vec3<f32>(0.0);
    var diffuse_sum = vec3<f32>(0.0);
    var specular_sum = vec3<f32>(0.0);

    for (var i = 0u; i < globals.num_lights; i++) {
      let light = globals.lights[i];
      var attenuation = 1.0;
      // Away from the light, which is what this formulation takes the dot products against.
      var away = light.direction;

      if (light.positional > 0.5) {
        let offset = world - light.position;
        let distance = length(offset);
        away = select(light.direction, offset / distance, distance > 1.0e-9);
        attenuation = 1.0 / max(light.attenuation.x + light.attenuation.y * distance +
          light.attenuation.z * distance * distance, 1.0e-9);
        // Per the OpenGL convention a spot light is one whose cone is 90 degrees or less.
        if (light.cone_angle <= 90.0) {
          let cone_dot = dot(away, light.direction);
          if (cone_dot >= cos(radians(light.cone_angle))) {
            attenuation = attenuation * pow(cone_dot, light.exponent);
          } else {
            attenuation = 0.0;
          }
        }
      }

      var n_dot_l = dot(normal, away);
      if (n_dot_l < 0.0 && two_sided) {
        n_dot_l = -n_dot_l;
      }
      if (n_dot_l > 0.0) {
        diffuse_sum += max(0.0, attenuation * n_dot_l) * light.diffuse;
        let reflected = normalize(2.0 * n_dot_l * normal - away);
        var r_dot_v = dot(-view, reflected);
        if (r_dot_v < 0.0 && two_sided) {
          r_dot_v = -r_dot_v;
        }
        if (r_dot_v > 0.0) {
          specular_sum += attenuation * pow(r_dot_v, params.specular_power) * light.specular;
        }
      }
      ambient_sum += light.ambient;
    }

    lit = params.ambient * ambient_sum + params.diffuse * diffuse_sum * color +
      params.specular * specular_sum;
  }

  return lit;
}

//------------------------------------------------------------------------------
// Ray setup
//------------------------------------------------------------------------------

fn unproject(ndc: vec2<f32>, depth: f32) -> vec3<f32> {
  let point = globals.inv_view_projection * vec4<f32>(ndc, depth, 1.0);
  return point.xyz / point.w;
}

fn intersectUnitBox(origin: vec3<f32>, direction: vec3<f32>) -> vec2<f32> {
  let inverse = 1.0 / direction;
  let t0 = (vec3<f32>(0.0) - origin) * inverse;
  let t1 = (vec3<f32>(1.0) - origin) * inverse;
  let t_min = min(t0, t1);
  let t_max = max(t0, t1);
  return vec2<f32>(max(max(t_min.x, t_min.y), t_min.z),
                   min(min(t_max.x, t_max.y), t_max.z));
}

// Clip the ray against the clipping planes of one input, in world space.
fn applyClippingPlanes(volume_index: u32, origin: vec3<f32>, direction: vec3<f32>,
                       range: vec2<f32>) -> vec2<f32> {
  var clipped = range;
  let count = volumes[volume_index].clip_plane_count;
  for (var i = 0u; i < count; i++) {
    let plane = volumes[volume_index].clip_planes[i];
    // Keep the half space where dot(normal, point) + offset >= 0.
    let denominator = dot(plane.xyz, direction);
    let numerator = dot(plane.xyz, origin) + plane.w;
    if (abs(denominator) < 1.0e-9) {
      if (numerator < 0.0) {
        return vec2<f32>(1.0, 0.0);
      }
    } else {
      let hit = -numerator / denominator;
      if (denominator > 0.0) {
        clipped.x = max(clipped.x, hit);
      } else {
        clipped.y = min(clipped.y, hit);
      }
    }
  }
  return clipped;
}

// The iso values, with an open interval either side so the march can walk off both ends.
fn isoValue(index: i32) -> f32 {
  if (index <= 0) {
    return -FAR_AWAY;
  }
  if (index > i32(globals.num_iso_values)) {
    return FAR_AWAY;
  }
  return globals.iso_values[index - 1].x;
}

// The interval a value falls in, counting from one.
fn findIsoInterval(value: f32) -> i32 {
  var index = 0;
  for (var i = 0u; i < globals.num_iso_values; i++) {
    if (value >= globals.iso_values[i].x) {
      index = i32(i) + 1;
    }
  }
  return index;
}

// Colour of one volume at a normalized scalar, shaded if the property asks for it.
fn classify(volume_index: u32, normalized: f32, tex_coord: vec3<f32>) -> vec4<f32> {
  let params = volumes[volume_index];
  let component = params.primary_component;
  let value = clamp(normalized, 0.0, 1.0);
  let opacity = opacityOf(volume_index, component, value);
  var color = colorOf(volume_index, component, value);
  if ((params.flags & VOLUME_FLAG_SHADE) != 0u && opacity > 0.0) {
    var gradient = computeGradient(volume_index, tex_coord);
    if ((params.flags & VOLUME_FLAG_NORMAL_FROM_OPACITY) != 0u) {
      gradient = computeOpacityGradient(volume_index, tex_coord);
    }
    color = shade(volume_index, color, tex_coord, gradient);
  }
  return vec4<f32>(color, opacity);
}

fn hash(p: vec2<f32>) -> f32 {
  var p3 = fract(vec3<f32>(p.x, p.y, p.x) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

//------------------------------------------------------------------------------
// Fragment stage
//------------------------------------------------------------------------------

struct RayState {
  color: vec3<f32>,
  alpha: f32,
  // Running scalar and its volume, used by the intensity blend modes.
  value: f32,
  value_volume: u32,
  count: f32,
}

@fragment
fn fragmentMain(@builtin(position) frag_coord: vec4<f32>) -> @location(0) vec4<f32> {
  let viewport = globals.viewport;
  let ndc = vec2<f32>((frag_coord.x - viewport.x) / viewport.z * 2.0 - 1.0,
                      1.0 - (frag_coord.y - viewport.y) / viewport.w * 2.0);

  let near_point = unproject(ndc, 0.0);
  let far_point = unproject(ndc, 1.0);
  let ray_direction = normalize(far_point - near_point);

  // Stop at the opaque geometry that was rasterized before this pass.
  var t_limit = distance(near_point, far_point);
  if ((globals.flags & FLAG_HAS_DEPTH) != 0u) {
    let depth = textureLoad(opaque_depth, vec2<i32>(frag_coord.xy), 0);
    if (depth < 1.0) {
      t_limit = min(t_limit, dot(unproject(ndc, depth) - near_point, ray_direction));
    }
  }
  if (t_limit <= 0.0) {
    discard;
  }

  var t_near_of = array<f32, MAX_VOLUMES>();
  var t_far_of = array<f32, MAX_VOLUMES>();
  var origin_of = array<vec3<f32>, MAX_VOLUMES>();
  var direction_of = array<vec3<f32>, MAX_VOLUMES>();

  var t_first = FAR_AWAY;
  var t_last = 0.0;

  for (var i = 0u; i < globals.num_volumes; i++) {
    let params = volumes[i];
    let origin = (params.inv_volume_matrix * vec4<f32>(near_point, 1.0)).xyz;
    let direction = (params.inv_volume_matrix * vec4<f32>(ray_direction, 0.0)).xyz;
    origin_of[i] = origin;
    direction_of[i] = direction;

    var span = intersectUnitBox(origin, direction);
    span = applyClippingPlanes(i, near_point, ray_direction, span);
    span.x = max(span.x, 0.0);
    span.y = min(span.y, t_limit);

    if (span.x < span.y) {
      t_near_of[i] = span.x;
      t_far_of[i] = span.y;
      t_first = min(t_first, span.x);
      t_last = max(t_last, span.y);
    } else {
      t_near_of[i] = FAR_AWAY;
      t_far_of[i] = 0.0;
    }
  }

  if (t_first >= t_last) {
    discard;
  }

  // A slice is not a march. Each volume contributes the single sample where the ray meets its
  // slice plane, and the samples are composited from the nearest one back.
  if (globals.blend_mode == BLEND_SLICE) {
    var taken = array<u32, MAX_VOLUMES>();
    var slice_color = vec3<f32>(0.0);
    var slice_alpha = 0.0;

    for (var pass_index = 0u; pass_index < globals.num_volumes; pass_index++) {
      var nearest = FAR_AWAY;
      var chosen = globals.num_volumes;
      for (var i = 0u; i < globals.num_volumes; i++) {
        if (taken[i] != 0u) {
          continue;
        }
        let plane = volumes[i].slice_plane;
        let denominator = dot(plane.xyz, ray_direction);
        if (abs(denominator) < 1.0e-9) {
          continue;
        }
        let hit = -(dot(plane.xyz, near_point) + plane.w) / denominator;
        if (hit < t_near_of[i] || hit > t_far_of[i] || hit >= nearest) {
          continue;
        }
        nearest = hit;
        chosen = i;
      }
      if (chosen >= globals.num_volumes) {
        break;
      }
      taken[chosen] = 1u;

      let tex_coord = origin_of[chosen] + nearest * direction_of[chosen];
      if ((pageOf(chosen, tex_coord).flags & PAGE_RESIDENT) == 0u ||
          cropped(chosen, tex_coord) || masked(chosen, tex_coord)) {
        continue;
      }
      let params = volumes[chosen];
      let scalar = sampleScalar(chosen, tex_coord);
      let normalized = normalizeComponent(chosen, params.primary_component, scalar);
      let sample = classify(chosen, normalized, tex_coord);
      let remaining = 1.0 - slice_alpha;
      slice_color += remaining * sample.a * sample.rgb;
      slice_alpha += remaining * sample.a;
    }

    if (slice_alpha <= 0.0) {
      discard;
    }
    return vec4<f32>(min(slice_color, vec3<f32>(1.0)), min(slice_alpha, 1.0));
  }

  var state: RayState;
  state.color = vec3<f32>(0.0);
  state.alpha = 0.0;
  state.value = select(-FAR_AWAY, FAR_AWAY, globals.blend_mode == BLEND_MINIMUM_INTENSITY);
  state.value_volume = 0u;
  state.count = 0.0;

  let adaptive = (globals.flags & FLAG_ADAPTIVE) != 0u;
  let skip_empty = adaptive && (globals.flags & FLAG_SKIP_EMPTY) != 0u;
  let compositing = globals.blend_mode == BLEND_COMPOSITE;
  let isosurface = globals.blend_mode == BLEND_ISOSURFACE;

  // Which interval between iso values each volume was last seen in, so that a crossing can be
  // recognised when it happens.
  var iso_interval = array<i32, MAX_VOLUMES>();
  var iso_seen = array<u32, MAX_VOLUMES>();

  var t = t_first;
  if ((globals.flags & FLAG_JITTER) != 0u) {
    t += hash(frag_coord.xy) * globals.jitter_scale;
  }

  var steps = 0u;
  while (t < t_last && steps < globals.max_steps) {
    steps++;

    // Which volumes contain this point, how finely they want to be sampled, and where the ray
    // could be advanced to if none of them has anything to contribute here.
    var step = FAR_AWAY;
    var skip_to = FAR_AWAY;
    var occupied = false;
    var any_active = false;

    for (var i = 0u; i < globals.num_volumes; i++) {
      if (t < t_near_of[i]) {
        skip_to = min(skip_to, t_near_of[i]);
        continue;
      }
      if (t >= t_far_of[i]) {
        continue;
      }
      any_active = true;

      let params = volumes[i];
      let tex_coord = origin_of[i] + t * direction_of[i];

      let page = pageOf(i, tex_coord);
      if ((page.flags & PAGE_RESIDENT) == 0u) {
        // The brick has not been brought in. Step over it rather than reading whatever the pool
        // happens to hold; it will be there once the mapper has had a chance to fetch it.
        skip_to = min(skip_to,
          t + blockExit(i, origin_of[i], direction_of[i], t, params.brick_size));
        continue;
      }
      if (adaptive) {
        let cell = cellAt(i, tex_coord);
        if (skip_empty && cell.x <= globals.empty_threshold) {
          skip_to = min(skip_to,
            t + blockExit(i, origin_of[i], direction_of[i], t, params.cell_size));
          continue;
        }
        occupied = true;
        step = min(step, params.base_step *
          mix(globals.max_step_factor, globals.min_step_factor, cell.y));
      } else {
        occupied = true;
        step = min(step, params.base_step);
      }
    }

    if (!occupied) {
      if (!any_active && skip_to >= FAR_AWAY) {
        break;
      }
      // Nothing here. Jump to wherever something starts, but never stand still.
      t = max(skip_to, t + max(1.0e-6, abs(t) * 1.0e-5));
      continue;
    }

    // Blend the volumes covering this point as media sharing it: opacities multiply and colours
    // are averaged by contribution, so the result does not depend on input connection order.
    var weighted_color = vec3<f32>(0.0);
    var weight_sum = 0.0;
    var transmittance = 1.0;

    for (var i = 0u; i < globals.num_volumes; i++) {
      if (t < t_near_of[i] || t >= t_far_of[i]) {
        continue;
      }

      let params = volumes[i];
      let tex_coord = origin_of[i] + t * direction_of[i];
      if ((pageOf(i, tex_coord).flags & PAGE_RESIDENT) == 0u) {
        continue;
      }
      if (cropped(i, tex_coord) || masked(i, tex_coord)) {
        continue;
      }
      let values = sampleVector(i, tex_coord);
      let primary = params.primary_component;
      let scalar = values[primary];
      let normalized = normalizeComponent(i, primary, scalar);

      if (isosurface) {
        // Compared in data units, unclamped: a surface may sit outside the transfer function
        // range, and clamping would collapse it onto an end and lose the crossing order.
        if (iso_seen[i] == 0u) {
          iso_interval[i] = findIsoInterval(scalar);
          iso_seen[i] = 1u;
          continue;
        }
        var crossing = 0.0;
        var crossed = false;
        var interval = clamp(iso_interval[i], 0, i32(globals.num_iso_values));
        if (scalar < isoValue(interval)) {
          crossing = isoValue(interval);
          interval -= 1;
          crossed = true;
        }
        if (scalar > isoValue(interval + 1)) {
          crossing = isoValue(interval + 1);
          interval += 1;
          crossed = true;
        }
        iso_interval[i] = interval;
        if (crossed) {
          let sample = classify(i, normalizeComponent(i, primary, crossing), tex_coord);
          let remaining = 1.0 - state.alpha;
          state.color += remaining * sample.a * sample.rgb;
          state.alpha += remaining * sample.a;
        }
        continue;
      }

      if (!compositing) {
        // The intensity modes work on the scalar rather than on a composited color.
        if (globals.blend_mode == BLEND_MAXIMUM_INTENSITY) {
          if (normalized > state.value) { state.value = normalized; state.value_volume = i; }
        } else if (globals.blend_mode == BLEND_MINIMUM_INTENSITY) {
          if (normalized < state.value) { state.value = normalized; state.value_volume = i; }
        } else {
          if (globals.blend_mode == BLEND_AVERAGE_INTENSITY &&
              (scalar < globals.average_range_minimum ||
               scalar > globals.average_range_maximum)) {
            continue;
          }
          if (state.count == 0.0) { state.value = 0.0; }
          // Independent components each contribute through their own transfer functions,
          // weighed against one another. Summing per sample needs no state.
          if ((params.flags & VOLUME_FLAG_INDEPENDENT_COMPONENTS) != 0u &&
              params.num_components > 1u) {
            var sum = 0.0;
            for (var c = 0u; c < params.num_components; c++) {
              let value = normalizeComponent(i, c, values[c]);
              sum += opacityOf(i, c, value) * value * params.component_weights[c];
            }
            state.value += sum;
          } else {
            state.value += opacityOf(i, primary, normalized) * normalized;
          }
          state.count += 1.0;
        }
        continue;
      }

      // The gradient is measured on the component the acceleration structure follows, which is
      // also the one that carries the surfaces worth shading.
      var gradient = vec3<f32>(0.0);
      var shading_gradient = vec3<f32>(0.0);
      let wants_gradient = (params.flags & (VOLUME_FLAG_SHADE | VOLUME_FLAG_GRADIENT_OPACITY)) != 0u;
      if (wants_gradient) {
        gradient = computeGradient(i, tex_coord);
        shading_gradient = gradient;
        if ((params.flags & VOLUME_FLAG_NORMAL_FROM_OPACITY) != 0u) {
          shading_gradient = computeOpacityGradient(i, tex_coord);
        }
      }

      var color = vec3<f32>(0.0);
      var opacity = 0.0;

      if ((params.flags & VOLUME_FLAG_INDEPENDENT_COMPONENTS) == 0u) {
        // Dependent components describe one colour: the last carries opacity, the rest are
        // colour channels. Two means the first is a scalar to colour, four means the first
        // three are the colour. Each component's tables cover its own data range.
        let last = params.num_components - 1u;
        opacity = opacityOf(i, last, normalizeComponent(i, last, values[last]));
        if (params.num_components == 2u) {
          color = colorOf(i, 0u, normalizeComponent(i, 0u, values[0]));
        } else {
          color = vec3<f32>(normalizeComponent(i, 0u, values[0]),
                            normalizeComponent(i, 1u, values[1]),
                            normalizeComponent(i, 2u, values[2]));
        }
        if (opacity > 0.0) {
          // The step correction belongs to the scalar opacity alone: the OpenGL mapper bakes it
          // into that table and leaves the gradient opacity table untouched.
          opacity = correctOpacity(opacity, step, params.opacity_unit_distance) *
            gradientOpacityOf(i, last, gradient, params.spacing);
        }
      } else if (params.num_components == 1u) {
        opacity = opacityOf(i, 0u, normalized);
        color = colorOf(i, 0u, normalized);
        if (opacity > 0.0) {
          opacity = correctOpacity(opacity, step, params.opacity_unit_distance) *
            gradientOpacityOf(i, 0u, gradient, params.spacing);
        }
      } else {
        // Independent components each carry their own transfer functions and are weighed
        // against one another, following vtkVolumeShaderComposer.
        var weighted = vec3<f32>(0.0);
        var total = 0.0;
        var alphas = vec4<f32>(0.0);
        for (var c = 0u; c < params.num_components; c++) {
          let value = normalizeComponent(i, c, values[c]);
          var component_opacity = opacityOf(i, c, value);
          if (component_opacity > 0.0) {
            component_opacity =
              correctOpacity(component_opacity, step, params.opacity_unit_distance) *
              gradientOpacityOf(i, c, gradient, params.spacing);
          }
          alphas[c] = component_opacity;
          total += component_opacity * params.component_weights[c];
        }
        if (total > 0.0) {
          for (var c = 0u; c < params.num_components; c++) {
            if (params.component_weights[c] <= 0.0) {
              continue;
            }
            let value = normalizeComponent(i, c, values[c]);
            weighted += colorOf(i, c, value) * alphas[c] * params.component_weights[c];
            opacity += alphas[c] * alphas[c] / total;
          }
          // The weighted sum above is premultiplied by the component opacities, so undo that to
          // get a colour the rest of the march can treat like any other.
          color = weighted / max(opacity, 1.0e-6);
        }
      }

      if (opacity <= 0.0) {
        continue;
      }
      opacity = clamp(opacity, 0.0, 1.0);

      var sample_color = color;
      if ((params.flags & VOLUME_FLAG_SHADE) != 0u) {
        sample_color = shade(i, color, tex_coord, shading_gradient);
      }

      weighted_color += opacity * sample_color;
      weight_sum += opacity;
      transmittance *= 1.0 - opacity;
    }

    if (weight_sum > 0.0) {
      let alpha = 1.0 - transmittance;
      let remaining = 1.0 - state.alpha;
      state.color += remaining * alpha * (weighted_color / weight_sum);
      state.alpha += remaining * alpha;
      if (state.alpha >= 0.99) {
        break;
      }
    }

    t += step;
  }

  if (!compositing && !isosurface) {
    if (globals.blend_mode == BLEND_MAXIMUM_INTENSITY ||
        globals.blend_mode == BLEND_MINIMUM_INTENSITY) {
      if (state.value <= -FAR_AWAY || state.value >= FAR_AWAY) {
        discard;
      }
      let index = state.value_volume;
      let component = volumes[index].primary_component;
      let value = clamp(state.value, 0.0, 1.0);
      let alpha = opacityOf(index, component, value);
      state.color = colorOf(index, component, value) * alpha;
      state.alpha = alpha;
    } else {
      // The summed and averaged modes report intensity, not color, and cover the proxy opaquely.
      if (state.count == 0.0) {
        discard;
      }
      if (globals.blend_mode == BLEND_AVERAGE_INTENSITY) {
        state.value /= state.count;
      }
      state.color = vec3<f32>(clamp(state.value, 0.0, 1.0));
      state.alpha = 1.0;
    }
  } else {
    state.color = min(state.color, vec3<f32>(1.0));
    state.alpha = min(state.alpha, 1.0);
  }

  if (state.alpha <= 0.0) {
    discard;
  }

  return vec4<f32>(state.color, state.alpha);
}
