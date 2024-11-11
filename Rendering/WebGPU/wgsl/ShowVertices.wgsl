// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// #include "VTK/wgsl/SceneTransform.wgsl"
// #include "VTK/wgsl/ActorTransform.wgsl"

// #include "VTK/wgsl/SceneLight.wgsl"

// #include "VTK/wgsl/ActorRenderOptions.wgsl"
// #include "VTK/wgsl/ActorColorOptions.wgsl"
// #include "VTK/wgsl/Utilities.wgsl"

const TRIANGLE_VERTS = array(
  vec2f(-1, -1),
  vec2f(1, -1),
  vec2f(-1, 1),
  vec2f(1, 1),
);

struct ActorBlock {
  transform: ActorTransform,
  render_options: ActorRenderOptions,
  color_options: ActorColorOptions
}

//-------------------------------------------------------------------
struct F32s {
  values: array<f32>
}

//-------------------------------------------------------------------
struct U32s {
  values: array<u32>
}

//-------------------------------------------------------------------
struct MeshAttributeArrayDescriptor {
  start: u32,
  num_tuples: u32,
  num_components: u32
}

//-------------------------------------------------------------------
struct OverrideColorDescriptor {
  apply_override_colors: u32,
  opacity: f32,
  ambient_color: vec3<f32>,
  diffuse_color: vec3<f32>
}

//-------------------------------------------------------------------
struct MeshDescriptor {
  position: MeshAttributeArrayDescriptor,
  point_color: MeshAttributeArrayDescriptor,
  point_normal: MeshAttributeArrayDescriptor,
  point_tangent: MeshAttributeArrayDescriptor,
  point_uv: MeshAttributeArrayDescriptor,
  cell_color: MeshAttributeArrayDescriptor,
  cell_normal: MeshAttributeArrayDescriptor,
  override_colors: OverrideColorDescriptor
}

///-----------------------------------------------------------------///
// Renderer controlled descriptor set
///-----------------------------------------------------------------///
@group(0) @binding(0) var<uniform> scene_transform: SceneTransform;
@group(0) @binding(1) var<storage, read> scene_lights: SceneLights;

///-------------------------------------------------------------------
// Everything shader needs from the vtkActor and it's vtkProperty
///-------------------------------------------------------------------
@group(1) @binding(0) var<uniform> actor: ActorBlock;

///-----------------------------------------------------------------///
// Mesh attributes.
///-----------------------------------------------------------------///
@group(2) @binding(0) var<storage, read> mesh: MeshDescriptor;
@group(2) @binding(1) var<storage, read> point_data: F32s;
@group(2) @binding(2) var<storage, read> cell_data: F32s;

//-------------------------------------------------------------------
struct VertexInput {
  @builtin(instance_index) instance_id: u32,
  @builtin(vertex_index) vertex_id: u32
}

//-------------------------------------------------------------------
struct VertexOutput {
  @builtin(position) position: vec4<f32>,
  @location(0) color: vec4<f32>,
  @location(1) position_vc: vec4<f32>,
  @location(2) local_position: vec2<f32>,
}

//-------------------------------------------------------------------
@vertex
fn vertexMain(vertex: VertexInput) -> VertexOutput {
  var output: VertexOutput;
  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let point_id = vertex.instance_id;
  // pull the position for this vertex.
  let p_mc = vec4<f32>(getTuple3F32(point_id, mesh.position.start, &point_data.values), 1.0);

  ///------------------------///
  // NDC transforms
  ///------------------------///
  // transform to view and then to clip space.
  let p_dc = scene_transform.projection * scene_transform.view * actor.transform.world * p_mc;

  // transform to 2-D screen plane.
  let resolution = scene_transform.viewport.zw;
  let p_screen = resolution * (0.5 * p_dc.xy / p_dc.w + 0.5);

  var point_size = actor.render_options.point_size;
  // The point rendering algorithm is unstable for point_size < 1.0
  if point_size < 1.0 {
    point_size = 1.0;
  }
  output.local_position = TRIANGLE_VERTS[vertex.vertex_id];
  let pscreen_offsetted = p_screen + 0.5 * point_size * output.local_position;
  output.position = vec4(p_dc.w * ((2.0 * pscreen_offsetted) / resolution - 1.0), p_dc.z, p_dc.w);
  output.position_vc = scene_transform.inverted_projection * output.position;
  return output;
}

//-------------------------------------------------------------------
struct FragmentInput {
  @builtin(position) frag_coord: vec4<f32>,
  @builtin(front_facing) is_front_facing: bool,
  @location(0) color: vec4<f32>,
  @location(1) position_vc: vec4<f32>, // in view coordinate system.
  @location(2) local_position: vec2<f32>,
}

//-------------------------------------------------------------------
struct FragmentOutput {
  @location(0) color: vec4<f32>,
}

//-------------------------------------------------------------------
@fragment
fn fragmentMain(fragment: FragmentInput) -> FragmentOutput {
  var output: FragmentOutput;
  var ambient_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var diffuse_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var specular_color: vec3<f32> = vec3<f32>(0., 0., 0.);

  let d = length(fragment.local_position); // distance of fragment from the input vertex in noramlized bi-unit domain.
  if ((actor.render_options.point_2d_shape == POINT_2D_ROUND) && (d > 1)) {
    discard;
  }

  var normal_vc = vec3f(0, 0, 1);
  if (actor.render_options.render_points_as_spheres != 0) {
    // adjust z component of normal in order to emulate a sphere if necessary.
    normal_vc.z = 1.0 - d;
  }

  var opacity: f32;

  ///------------------------///
  // Colors are acquired from the vtkProperty
  ///------------------------///
  ambient_color = actor.color_options.vertex_color;
  diffuse_color = actor.color_options.vertex_color;
  opacity = actor.color_options.opacity;

  ///------------------------///
  // Lights
  ///------------------------///
  if scene_lights.count == 0u {
    // allow post-processing this pixel.
    output.color = vec4<f32>(
      actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
      opacity
    );
  } else if scene_lights.count == 1u {
    let light: SceneLight = scene_lights.values[0];
    if light.positional == 1u {
      // TODO: positional
      output.color = vec4<f32>(
          actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
          opacity
      );
    } else {
      // headlight
      let df: f32 = max(0.000001f, normal_vc.z);
      let sf: f32 = pow(df, actor.color_options.specular_power);
      diffuse_color = df * diffuse_color * light.color;
      specular_color = sf * actor.color_options.specular_intensity * actor.color_options.specular_color * light.color;
      output.color = vec4<f32>(
          actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color + specular_color,
          opacity
      );
    }
  } else {
    // TODO: light kit
    output.color = vec4<f32>(
      actor.color_options.ambient_intensity * ambient_color + actor.color_options.diffuse_intensity * diffuse_color,
      opacity
    );
  }
  // pre-multiply colors
  output.color = vec4(output.color.rgb * opacity, opacity);
  return output;
}
