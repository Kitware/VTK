// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Ray casts any number of vtkImageData volumes in a single pass.
//
// Rays are parameterised by world space distance from the near plane, so the parameter is shared
// by every volume no matter how each one is placed. That is what lets overlapping inputs be
// composited correctly: at each step the shader collects the volumes that contain the current
// point, blends them as co-located participating media, and composites the result once.

const MAX_VOLUMES: u32 = 12u;
const MAX_LIGHTS: u32 = 6u;
const MAX_CLIP_PLANES: u32 = 6u;

// GlobalUniforms.flags
const FLAG_JITTER: u32 = 1u;
const FLAG_ADAPTIVE: u32 = 2u;
const FLAG_SKIP_EMPTY: u32 = 4u;
const FLAG_FULL_SCREEN: u32 = 8u;
const FLAG_HAS_DEPTH: u32 = 16u;

// VolumeParams.flags
const VOLUME_FLAG_SHADE: u32 = 1u;
const VOLUME_FLAG_GRADIENT_OPACITY: u32 = 2u;

// GlobalUniforms.blend_mode, matching vtkVolumeMapper::BlendModes
const BLEND_COMPOSITE: u32 = 0u;
const BLEND_MAXIMUM_INTENSITY: u32 = 1u;
const BLEND_MINIMUM_INTENSITY: u32 = 2u;
const BLEND_AVERAGE_INTENSITY: u32 = 3u;
const BLEND_ADDITIVE: u32 = 4u;

const FAR_AWAY: f32 = 3.4e38;

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
  num_clip_planes: u32,
  padding0: f32,
  // Window the average intensity mode restricts itself to, in data units.
  average_range_minimum: f32,
  average_range_maximum: f32,
  padding1: vec2<f32>,
  light_directions: array<vec4<f32>, MAX_LIGHTS>,
  light_colors: array<vec4<f32>, MAX_LIGHTS>,
  clip_planes: array<vec4<f32>, MAX_CLIP_PLANES>,
}

struct VolumeParams {
  // texture space [0,1]^3 to world space, and back
  volume_matrix: mat4x4<f32>,
  inv_volume_matrix: mat4x4<f32>,
  dims: vec3<f32>,
  base_step: f32,
  spacing: vec3<f32>,
  value_scale: f32,
  brick_dims: vec3<f32>,
  value_bias: f32,
  scalar_range_min: f32,
  scalar_range_scale: f32,
  brick_offset: u32,
  brick_size: f32,
  opacity_unit_distance: f32,
  ambient: f32,
  diffuse: f32,
  specular: f32,
  specular_power: f32,
  gradient_opacity_min: f32,
  gradient_opacity_scale: f32,
  flags: u32,
}

@group(0) @binding(0) var<uniform> globals: GlobalUniforms;
@group(0) @binding(1) var<uniform> volumes: array<VolumeParams, MAX_VOLUMES>;
// One entry per block of every volume: x is the largest opacity the transfer functions map the
// block to, y is how much the block varies, in [0,1].
@group(0) @binding(2) var<storage, read> bricks: array<vec2<f32>>;
@group(0) @binding(3) var color_transfer_functions: texture_2d_array<f32>;
@group(0) @binding(4) var opacity_transfer_functions: texture_2d_array<f32>;
@group(0) @binding(5) var gradient_transfer_functions: texture_2d_array<f32>;
@group(0) @binding(6) var transfer_function_sampler: sampler;
@group(0) @binding(7) var opaque_depth: texture_depth_2d;
@group(0) @binding(8) var volume_texture_0: texture_3d<f32>;
@group(0) @binding(9) var volume_texture_1: texture_3d<f32>;
@group(0) @binding(10) var volume_texture_2: texture_3d<f32>;
@group(0) @binding(11) var volume_texture_3: texture_3d<f32>;
@group(0) @binding(12) var volume_texture_4: texture_3d<f32>;
@group(0) @binding(13) var volume_texture_5: texture_3d<f32>;
@group(0) @binding(14) var volume_texture_6: texture_3d<f32>;
@group(0) @binding(15) var volume_texture_7: texture_3d<f32>;
@group(0) @binding(16) var volume_texture_8: texture_3d<f32>;
@group(0) @binding(17) var volume_texture_9: texture_3d<f32>;
@group(0) @binding(18) var volume_texture_10: texture_3d<f32>;
@group(0) @binding(19) var volume_texture_11: texture_3d<f32>;

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

fn loadTexel(volume_index: u32, coord: vec3<i32>) -> f32 {
  switch volume_index {
    case 0u: { return textureLoad(volume_texture_0, coord, 0).r; }
    case 1u: { return textureLoad(volume_texture_1, coord, 0).r; }
    case 2u: { return textureLoad(volume_texture_2, coord, 0).r; }
    case 3u: { return textureLoad(volume_texture_3, coord, 0).r; }
    case 4u: { return textureLoad(volume_texture_4, coord, 0).r; }
    case 5u: { return textureLoad(volume_texture_5, coord, 0).r; }
    case 6u: { return textureLoad(volume_texture_6, coord, 0).r; }
    case 7u: { return textureLoad(volume_texture_7, coord, 0).r; }
    case 8u: { return textureLoad(volume_texture_8, coord, 0).r; }
    case 9u: { return textureLoad(volume_texture_9, coord, 0).r; }
    case 10u: { return textureLoad(volume_texture_10, coord, 0).r; }
    case 11u: { return textureLoad(volume_texture_11, coord, 0).r; }
    default: { return 0.0; }
  }
}

// Trilinear interpolation by hand. The volume textures are bound as unfilterable so that both
// 8 bit and 32 bit float volumes can share one bind group layout.
fn sampleScalar(volume_index: u32, tex_coord: vec3<f32>) -> f32 {
  let params = volumes[volume_index];
  let last = vec3<i32>(params.dims) - vec3<i32>(1);
  let voxel = tex_coord * (params.dims - vec3<f32>(1.0));
  let base = floor(voxel);
  let frac = voxel - base;

  let i0 = clamp(vec3<i32>(base), vec3<i32>(0), last);
  let i1 = min(i0 + vec3<i32>(1), last);

  let c000 = loadTexel(volume_index, vec3<i32>(i0.x, i0.y, i0.z));
  let c100 = loadTexel(volume_index, vec3<i32>(i1.x, i0.y, i0.z));
  let c010 = loadTexel(volume_index, vec3<i32>(i0.x, i1.y, i0.z));
  let c110 = loadTexel(volume_index, vec3<i32>(i1.x, i1.y, i0.z));
  let c001 = loadTexel(volume_index, vec3<i32>(i0.x, i0.y, i1.z));
  let c101 = loadTexel(volume_index, vec3<i32>(i1.x, i0.y, i1.z));
  let c011 = loadTexel(volume_index, vec3<i32>(i0.x, i1.y, i1.z));
  let c111 = loadTexel(volume_index, vec3<i32>(i1.x, i1.y, i1.z));

  let c00 = mix(c000, c100, frac.x);
  let c10 = mix(c010, c110, frac.x);
  let c01 = mix(c001, c101, frac.x);
  let c11 = mix(c011, c111, frac.x);
  let raw = mix(mix(c00, c10, frac.y), mix(c01, c11, frac.y), frac.z);

  return raw * params.value_scale + params.value_bias;
}

// Central differences, returned in texture space. Divide component wise by twice the spacing to
// get the gradient with respect to world distance.
fn computeGradient(volume_index: u32, tex_coord: vec3<f32>) -> vec3<f32> {
  let params = volumes[volume_index];
  let step = 1.0 / max(params.dims - vec3<f32>(1.0), vec3<f32>(1.0));
  let xp = sampleScalar(volume_index, tex_coord + vec3<f32>(step.x, 0.0, 0.0));
  let xm = sampleScalar(volume_index, tex_coord - vec3<f32>(step.x, 0.0, 0.0));
  let yp = sampleScalar(volume_index, tex_coord + vec3<f32>(0.0, step.y, 0.0));
  let ym = sampleScalar(volume_index, tex_coord - vec3<f32>(0.0, step.y, 0.0));
  let zp = sampleScalar(volume_index, tex_coord + vec3<f32>(0.0, 0.0, step.z));
  let zm = sampleScalar(volume_index, tex_coord - vec3<f32>(0.0, 0.0, step.z));
  return vec3<f32>(xp - xm, yp - ym, zp - zm);
}

fn brickAt(volume_index: u32, tex_coord: vec3<f32>) -> vec2<f32> {
  let params = volumes[volume_index];
  let voxel = tex_coord * (params.dims - vec3<f32>(1.0));
  let block = clamp(floor(voxel / params.brick_size), vec3<f32>(0.0),
                    params.brick_dims - vec3<f32>(1.0));
  let index = params.brick_offset +
    u32(block.z * params.brick_dims.y * params.brick_dims.x +
        block.y * params.brick_dims.x + block.x);
  return bricks[index];
}

// Distance, along the ray, from the current point to where it leaves the block it is in.
fn brickExit(volume_index: u32, origin: vec3<f32>, direction: vec3<f32>, t: f32) -> f32 {
  let params = volumes[volume_index];
  let extent = max(params.dims - vec3<f32>(1.0), vec3<f32>(1.0));
  let voxel = (origin + t * direction) * extent;
  let block = floor(voxel / params.brick_size);

  let lower = block * params.brick_size / extent;
  let upper = (block + vec3<f32>(1.0)) * params.brick_size / extent;

  let inverse = 1.0 / direction;
  let t0 = (lower - origin) * inverse;
  let t1 = (upper - origin) * inverse;
  let t_max = max(t0, t1);
  return max(min(min(t_max.x, t_max.y), t_max.z) - t, 0.0);
}

//------------------------------------------------------------------------------
// Shading
//------------------------------------------------------------------------------

fn shade(volume_index: u32, color: vec3<f32>, tex_coord: vec3<f32>,
         gradient: vec3<f32>) -> vec3<f32> {
  let params = volumes[volume_index];
  let magnitude = length(gradient);
  if (magnitude < 1.0e-6) {
    return color;
  }

  // Fade the lighting in as the gradient grows. In nearly uniform regions its direction is noise
  // and shading it produces speckle.
  let blend = smoothstep(0.005, 0.05, magnitude);

  var normal = normalize((transpose(params.inv_volume_matrix) * vec4<f32>(gradient, 0.0)).xyz);
  let world = (params.volume_matrix * vec4<f32>(tex_coord, 1.0)).xyz;
  let view = normalize(globals.camera_position - world);
  if (dot(normal, view) < 0.0) {
    normal = -normal;
  }

  var lit = color * params.ambient;
  for (var i = 0u; i < globals.num_lights; i++) {
    let to_light = normalize(globals.light_directions[i].xyz);
    let halfway = normalize(to_light + view);
    let diffuse = max(dot(normal, to_light), 0.0);
    let specular = select(
      0.0, pow(max(dot(normal, halfway), 0.0), params.specular_power), diffuse > 0.0);
    let light_color = globals.light_colors[i].xyz;
    lit += color * params.diffuse * diffuse * light_color;
    lit += params.specular * specular * light_color;
  }

  return mix(color, lit, blend);
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

// Clip the ray against the mapper clipping planes, in world space.
fn applyClippingPlanes(origin: vec3<f32>, direction: vec3<f32>, range: vec2<f32>) -> vec2<f32> {
  var clipped = range;
  for (var i = 0u; i < globals.num_clip_planes; i++) {
    let plane = globals.clip_planes[i];
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
    span = applyClippingPlanes(near_point, ray_direction, span);
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

  var state: RayState;
  state.color = vec3<f32>(0.0);
  state.alpha = 0.0;
  state.value = select(-FAR_AWAY, FAR_AWAY, globals.blend_mode == BLEND_MINIMUM_INTENSITY);
  state.value_volume = 0u;
  state.count = 0.0;

  let adaptive = (globals.flags & FLAG_ADAPTIVE) != 0u;
  let skip_empty = adaptive && (globals.flags & FLAG_SKIP_EMPTY) != 0u;
  let compositing = globals.blend_mode == BLEND_COMPOSITE;

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

      if (adaptive) {
        let brick = brickAt(i, tex_coord);
        if (skip_empty && brick.x <= globals.empty_threshold) {
          skip_to = min(skip_to, t + brickExit(i, origin_of[i], direction_of[i], t));
          continue;
        }
        occupied = true;
        step = min(step, params.base_step *
          mix(globals.max_step_factor, globals.min_step_factor, brick.y));
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
      let scalar = sampleScalar(i, tex_coord);
      let normalized =
        clamp((scalar - params.scalar_range_min) * params.scalar_range_scale, 0.0, 1.0);

      let lookup = vec2<f32>(normalized, 0.5);

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
          let weight = textureSampleLevel(
            opacity_transfer_functions, transfer_function_sampler, lookup, i, 0.0).r;
          state.value += weight * normalized;
          state.count += 1.0;
        }
        continue;
      }

      let color =
        textureSampleLevel(color_transfer_functions, transfer_function_sampler, lookup, i, 0.0).rgb;
      var opacity = textureSampleLevel(
        opacity_transfer_functions, transfer_function_sampler, lookup, i, 0.0).r;

      var gradient = vec3<f32>(0.0);
      let wants_gradient = (params.flags & (VOLUME_FLAG_SHADE | VOLUME_FLAG_GRADIENT_OPACITY)) != 0u;
      if (wants_gradient && opacity > 0.0) {
        gradient = computeGradient(i, tex_coord);
      }

      if ((params.flags & VOLUME_FLAG_GRADIENT_OPACITY) != 0u && opacity > 0.0) {
        let physical = length(gradient / (2.0 * params.spacing));
        let position = clamp(
          (physical - params.gradient_opacity_min) * params.gradient_opacity_scale, 0.0, 1.0);
        opacity *= textureSampleLevel(gradient_transfer_functions, transfer_function_sampler,
          vec2<f32>(position, 0.5), i, 0.0).r;
      }

      if (opacity <= 0.0) {
        continue;
      }

      // The transfer function defines opacity over one unit distance, so correct it for how far
      // this step actually travels.
      opacity = 1.0 - pow(1.0 - min(opacity, 0.9999), step / params.opacity_unit_distance);

      var sample_color = color;
      if ((params.flags & VOLUME_FLAG_SHADE) != 0u) {
        sample_color = shade(i, color, tex_coord, gradient);
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

  if (!compositing) {
    if (globals.blend_mode == BLEND_MAXIMUM_INTENSITY ||
        globals.blend_mode == BLEND_MINIMUM_INTENSITY) {
      if (state.value <= -FAR_AWAY || state.value >= FAR_AWAY) {
        discard;
      }
      let lookup = vec2<f32>(clamp(state.value, 0.0, 1.0), 0.5);
      let index = state.value_volume;
      let alpha = textureSampleLevel(
        opacity_transfer_functions, transfer_function_sampler, lookup, index, 0.0).r;
      state.color = textureSampleLevel(
        color_transfer_functions, transfer_function_sampler, lookup, index, 0.0).rgb * alpha;
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
