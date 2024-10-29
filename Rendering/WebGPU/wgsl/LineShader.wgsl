// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// #include "VTK/wgsl/SceneTransform.wgsl"
// #include "VTK/wgsl/ActorTransform.wgsl"

// #include "VTK/wgsl/SceneLight.wgsl"

// #include "VTK/wgsl/ActorRenderOptions.wgsl"
// #include "VTK/wgsl/ActorColorOptions.wgsl"
// #include "VTK/wgsl/Utilities.wgsl"

/**
 * (0, 0.5) |-------------------------------|(1, 0.5)
 *          |-                              |
 *          |    -                          |
 *          |        -                      |
 * (0, 0)   |              -                |
 *          |                   -           |
 *          |                        -      |
 *          |                              -|
 * (0,-0.5) |-------------------------------|(1, -0.5)
 */
const TRIANGLE_VERTS = array(
  vec2(0, -0.5),
  vec2(1, -0.5),
  vec2(0, 0.5),
  vec2(1, 0.5),
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

//-------------------------------------------------------------------
struct Topology {
  // the vtk cell ID for this index. used to index into a cell attribute.
  cell_id: u32,
  // the vtk point ID for this index. used to index into a point attribute.
  point_id: u32,
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

///-----------------------------------------------------------------///
// Topology
///-----------------------------------------------------------------///
@group(3) @binding(0) var<storage, read> topology: array<Topology>;

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
  @location(2) normal_vc: vec3<f32>,
  @location(3) tangent_vc: vec3<f32>,
  @location(4) @interpolate(flat) cell_id: u32,
  @location(5) local_position: vec2<f32>,
}


//-------------------------------------------------------------------
@vertex
fn vertexMain(vertex: VertexInput) -> VertexOutput {
  var output: VertexOutput;
  let line_segment_id: u32 = vertex.instance_id;

  var width: f32 = actor.render_options.line_width;
  if (width < 1.0)
  {
    // lines thinner than 1 pixel don't look good.
    width = 1.0;
  }
  let local_position = TRIANGLE_VERTS[vertex.vertex_id];

  let p0_vertex_id: u32 = 2 * line_segment_id;
  let p1_vertex_id = p0_vertex_id + 1;

  let p0_point_id: u32 = topology[p0_vertex_id].point_id;
  let p1_point_id: u32 = topology[p1_vertex_id].point_id;

  let p0_mc = vec4(getTuple3F32(p0_point_id, mesh.position.start, &point_data.values), 1);
  let p1_mc = vec4(getTuple3F32(p1_point_id, mesh.position.start, &point_data.values), 1);

  // transform to view and then to clip space.
  let p0_dc = scene_transform.projection * scene_transform.view * actor.transform.world * p0_mc;
  let p1_dc = scene_transform.projection * scene_transform.view * actor.transform.world * p1_mc;

  // transform to 2-D screen plane.
  let resolution = scene_transform.viewport.zw;
  let p0_screen = resolution * (0.5 * p0_dc.xy / p0_dc.w + 0.5);
  let p1_screen = resolution * (0.5 * p1_dc.xy / p1_dc.w + 0.5);

  let x_basis = normalize(p1_screen - p0_screen);
  let y_basis = vec2(-x_basis.y, x_basis.x);

  let p0_offsetted = p0_screen + local_position.x * x_basis + local_position.y * y_basis * width;
  let p1_offsetted = p1_screen + local_position.x * x_basis + local_position.y * y_basis * width;
  let p = mix(p0_offsetted, p1_offsetted, local_position.x);

  // used to select the z, w coordinate.
  let p_dc = mix(p0_dc, p1_dc, local_position.x);

  output.position = vec4(p_dc.w * ((2.0 * p) / resolution - 1.0), p_dc.z, p_dc.w);
  output.position_vc = scene_transform.inverted_projection * output.position;
  output.local_position = local_position;

  // pull the point id
  let point_id = select(topology[p0_vertex_id].point_id, topology[p1_vertex_id].point_id, local_position.x == 1);
  // get CellID from vertex ID -> VTK cell map.
  output.cell_id = select(topology[p0_vertex_id].cell_id, topology[p1_vertex_id].cell_id, local_position.x == 1);

  ///------------------------///
  // Smooth/Flag shading
  ///------------------------///
  if mesh.point_color.num_tuples > 0 {
    // Smooth shading
    output.color = getTuple4F32(point_id, mesh.point_color.start, &point_data.values);
  } else if mesh.cell_color.num_tuples > 0u {
    // Flat shading
    output.color = getTuple4F32(output.cell_id, mesh.cell_color.start, &cell_data.values);
  }

  ///------------------------///
  // Set Normals/Tangents
  // Basically infers what kind of directional vectors are available to use in lighting calculations.
  ///------------------------///
  if mesh.cell_normal.num_tuples > 0u {
    // pull normal of this vertex from cell normals
    let normal_mc = getTuple3F32(output.cell_id, mesh.cell_normal.start, &cell_data.values);
    output.normal_vc = scene_transform.normal * actor.transform.normal * normal_mc;
  } else if mesh.point_tangent.num_tuples > 0u {
    // pull tangent of this vertex from point tangents
    let tangentMC = getTuple3F32(point_id, mesh.point_tangent.start, &point_data.values);
    output.tangent_vc = scene_transform.normal * actor.transform.normal * tangentMC;
  } else if mesh.point_normal.num_tuples > 0u {
    // this if is after cell normals, so that when both are available, point normals are used.
    // pull normal of this vertex from point normals
    let normal_mc = getTuple3F32(point_id, mesh.point_normal.start, &point_data.values);
    output.normal_vc = scene_transform.normal * actor.transform.normal * normal_mc;
  } else {
    output.normal_vc = vec3<f32>(0.0, 0.0, 1.0);
  }
  return output;
}

//-------------------------------------------------------------------
struct FragmentInput {
  @builtin(position) frag_coord: vec4<f32>,
  @builtin(front_facing) is_front_facing: bool,
  @location(0) color: vec4<f32>,
  @location(1) position_vc: vec4<f32>, // in view coordinate system.
  @location(2) normal_vc: vec3<f32>, // in view coordinate system.
  @location(3) tangent_vc: vec3<f32>, // in view coordinate system.
  @location(4) @interpolate(flat) cell_id: u32,
  @location(5) local_position: vec2<f32>,
}

//-------------------------------------------------------------------
struct FragmentOutput {
  @location(0) color: vec4<f32>,
  @location(1) cell_id: u32
}

//-------------------------------------------------------------------
@fragment
fn fragmentMain(fragment: FragmentInput) -> FragmentOutput {
  var output: FragmentOutput;
  var ambient_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var diffuse_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var specular_color: vec3<f32> = vec3<f32>(0., 0., 0.);

  var opacity: f32;

  let distance_to_centerline = abs(fragment.local_position.y);

  // adjust z component of normal in order to emulate a tube if necessary.
  var normal_vc: vec3<f32> = normalize(fragment.normal_vc);
  if (actor.render_options.render_lines_as_tubes != 0)
  {
    normal_vc.z = 1.0 - 2.0 * distance_to_centerline;
  }

  ///------------------------///
  // Colors are acquired either from a global per-actor color, or from per-vertex colors, or from cell colors.
  ///------------------------///
  let has_mapped_colors: bool = mesh.point_color.num_tuples > 0u || mesh.cell_color.num_tuples > 0u;
  if (mesh.override_colors.apply_override_colors == 1u) {
    ambient_color = mesh.override_colors.ambient_color.rgb;
    diffuse_color = mesh.override_colors.diffuse_color.rgb;
    opacity = mesh.override_colors.opacity;
  } else if (has_mapped_colors) {
    ambient_color = fragment.color.rgb;
    diffuse_color = fragment.color.rgb;
    opacity = fragment.color.a;
  } else {
    ambient_color = actor.color_options.ambient_color;
    diffuse_color = actor.color_options.diffuse_color;
    opacity = actor.color_options.opacity;
  }

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
  output.cell_id = fragment.cell_id;
  return output;
}
