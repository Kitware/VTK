// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// #include "VTK/wgsl/SceneTransform.wgsl"
// #include "VTK/wgsl/ActorTransform.wgsl"

// #include "VTK/wgsl/SceneLight.wgsl"

// #include "VTK/wgsl/ActorRenderOptions.wgsl"
// #include "VTK/wgsl/ActorColorOptions.wgsl"
// #include "VTK/wgsl/Utilities.wgsl"


struct ActorBlock {
  transform: ActorTransform,
  render_options: ActorRenderOptions,
  color_options: ActorColorOptions,
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
struct AttributeArrayDescriptor {
  start: u32,
  num_tuples: u32,
  num_components: u32
}

//-------------------------------------------------------------------
struct MeshDescriptor {
  position: AttributeArrayDescriptor,
  point_normal: AttributeArrayDescriptor,
  point_tangent: AttributeArrayDescriptor,
  point_uv: AttributeArrayDescriptor,
  cell_normal: AttributeArrayDescriptor,
  instance_colors: AttributeArrayDescriptor,
  instance_transforms: AttributeArrayDescriptor,
  instance_normal_transforms: AttributeArrayDescriptor,
  composite_id: u32,
  process_id: u32,
  pickable: u32,
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
@group(1) @binding(0) var<storage, read> actor: ActorBlock;

///-----------------------------------------------------------------///
// Mesh attributes.
///-----------------------------------------------------------------///
@group(2) @binding(0) var<storage, read> mesh: MeshDescriptor;
@group(2) @binding(1) var<storage, read> point_data: F32s;
@group(2) @binding(2) var<storage, read> cell_data: F32s;
@group(2) @binding(3) var<storage, read> instance_data: F32s;

///-----------------------------------------------------------------///
// Topology
///-----------------------------------------------------------------///
@group(3) @binding(0) var<storage, read> topology: array<Topology>;
@group(3) @binding(1) var<storage, read> edge_array: array<f32>;


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
  @location(4) edge_dists: vec3<f32>,
  @location(5) @interpolate(flat) hide_edge: f32,
  @location(6) @interpolate(flat) cell_id: u32,
  @location(7) @interpolate(flat) prop_id: u32,
  @location(8) @interpolate(flat) composite_id: u32,
  @location(9) @interpolate(flat) process_id: u32,
}

//-------------------------------------------------------------------
@vertex
fn vertexMain(vertex: VertexInput) -> VertexOutput {
  var output: VertexOutput;
  let glyph_transform = getMat4F32(vertex.instance_id, mesh.instance_transforms.start, &instance_data.values);
  let glyph_normal_transform = getMat3F32(vertex.instance_id, mesh.instance_normal_transforms.start, &instance_data.values);

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.vertex_id;
  // get CellID from vertex ID -> VTK cell map.
  let cell_id = topology[pull_vertex_id].cell_id;
  // pull the point id
  let point_id = topology[pull_vertex_id].point_id;
  // pull the position for this vertex.
  let vertex_mc = vec4<f32>(getTuple3F32(point_id, mesh.position.start, &point_data.values), 1.0);

  // Write indices
  output.cell_id = cell_id;
  output.prop_id = actor.color_options.id;
  output.composite_id = mesh.composite_id;
  output.process_id = 0u;

  ///------------------------///
  // NDC transforms
  ///------------------------///
  output.position_vc = scene_transform.view * actor.transform.world * glyph_transform * vertex_mc;
  output.position = scene_transform.projection * output.position_vc;

  ///------------------------///
  // color
  ///------------------------///
  output.color = getTuple4F32(vertex.instance_id, mesh.instance_colors.start, &instance_data.values);

  ///------------------------///
  // Representation: VTK_SURFACE + Edge visibility turned on
  ///------------------------///
  let representation = getRepresentation(actor.render_options.flags);
  let edge_visibility = getEdgeVisibility(actor.render_options.flags);
  if (representation == VTK_SURFACE && edge_visibility) {
    let triangle_id: u32 = pull_vertex_id / 3u;
    let i0 = triangle_id * 3u;
    let pt0 = topology[i0].point_id;
    let pt1 = topology[i0 + 1u].point_id;
    let pt2 = topology[i0 + 2u].point_id;
    let p0_mc = vec4<f32>(getTuple3F32(pt0, mesh.position.start, &point_data.values), 1);
    let p1_mc = vec4<f32>(getTuple3F32(pt1, mesh.position.start, &point_data.values), 1);
    let p2_mc = vec4<f32>(getTuple3F32(pt2, mesh.position.start, &point_data.values), 1);
    let mvp = scene_transform.projection * scene_transform.view * actor.transform.world * glyph_transform;
    let p0_3d: vec4<f32> = mvp * p0_mc;
    let p1_3d: vec4<f32> = mvp * p1_mc;
    let p2_3d: vec4<f32> = mvp * p2_mc;
    let p0: vec2<f32> = p0_3d.xy / p0_3d.w;
    let p1: vec2<f32> = p1_3d.xy / p1_3d.w;
    let p2: vec2<f32> = p2_3d.xy / p2_3d.w;
    let use_id: u32 = pull_vertex_id % 3u;
    let win_scale = scene_transform.viewport.zw * 0.5;
    let edge_value: f32 = edge_array[triangle_id];
    if use_id == 0u {
      let v10 = win_scale * (p1 - p0);
      let v20 = win_scale * (p2 - p0) ;
      let area0: f32 = abs(v10.x * v20.y - v10.y * v20.x);
      let h0: f32 = area0 / length(v10 - v20);
      output.edge_dists = vec3<f32>(h0 * p0_3d.w, 0.0, 0.0);
    } else if use_id == 1u {
      let v01 = win_scale * (p0 - p1);
      let v21 = win_scale * (p2 - p1) ;
      let area1: f32 = abs(v01.x * v21.y - v01.y * v21.x);
      let h1: f32 = area1 / length(v01 - v21);
      output.edge_dists = vec3<f32>(0.0, h1 * p1_3d.w, 0.0);
    } else if use_id == 2u {
      let v02 = win_scale * (p0 - p2);
      let v12 = win_scale * (p1 - p2) ;
      let area2: f32 = abs(v02.x * v12.y - v02.y * v12.x);
      let h2: f32 = area2 / length(v02 - v12);
      output.edge_dists = vec3<f32>(0.0, 0.0, h2 * p2_3d.w);
    }
    output.hide_edge = edge_value;
  }
  ///------------------------///
  // Set Normals/Tangents
  // Basically infers what kind of directional vectors are available to use in lighting calculations.
  ///------------------------///
  if mesh.cell_normal.num_tuples > 0u {
    // pull normal of this vertex from cell normals
    let normal_mc = getTuple3F32(cell_id, mesh.cell_normal.start, &cell_data.values);
    output.normal_vc = scene_transform.normal * actor.transform.normal * glyph_normal_transform * normal_mc;
  } else if mesh.point_tangent.num_tuples > 0u {
    // pull tangent of this vertex from point tangents
    let tangentMC = getTuple3F32(point_id, mesh.point_tangent.start, &point_data.values);
    output.tangent_vc = scene_transform.normal * actor.transform.normal * glyph_normal_transform * tangentMC;
  } else if mesh.point_normal.num_tuples > 0u {
    // this if is after cell normals, so that when both are available, point normals are used.
    // pull normal of this vertex from point normals
    let normal_mc = getTuple3F32(point_id, mesh.point_normal.start, &point_data.values);
    output.normal_vc = scene_transform.normal * actor.transform.normal * glyph_normal_transform * normal_mc;
  } else {
    let next_id: u32 = (pull_vertex_id + 1u) % 3u;
    let prev_id: u32 = (pull_vertex_id + 2u) % 3u;
    let triangle_id: u32 = pull_vertex_id / 3u;
    let next_pt_id = topology[triangle_id * 3u + next_id].point_id;
    let prev_pt_id = topology[triangle_id * 3u + prev_id].point_id;
    let next_mc: vec3<f32> = getTuple3F32(next_pt_id, mesh.position.start, &point_data.values);
    let prev_mc: vec3<f32> = getTuple3F32(prev_pt_id, mesh.position.start, &point_data.values);
    let normal_mc = computeFaceNormal(vertex_mc.xyz, next_mc, prev_mc);
    output.normal_vc = scene_transform.normal * actor.transform.normal * glyph_normal_transform * normal_mc;
  }
  return output;
}

//-------------------------------------------------------------------
struct FragmentOutput {
  @location(0) color: vec4<f32>,
  @location(1) ids: vec4<u32>, // cell_id, prop_id, composite_id, process_id
}

//-------------------------------------------------------------------
@fragment
fn fragmentMain(
  @builtin(front_facing) is_front_facing: bool,
  vertex: VertexOutput) -> FragmentOutput {
  var output: FragmentOutput;
  var ambient_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var diffuse_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var specular_color: vec3<f32> = vec3<f32>(0., 0., 0.);
  var normal_vc: vec3<f32> = normalize(vertex.normal_vc);

  var opacity: f32;

  if (mesh.pickable == 1u)
  {
    output.ids.x = vertex.cell_id + 1;
    output.ids.y = vertex.prop_id + 1;
    output.ids.z = vertex.composite_id + 1;
    output.ids.w = vertex.process_id + 1;
  }

  ///------------------------///
  // Colors are acquired either from a global per-actor color, or from per-vertex colors, or from cell colors.
  ///------------------------///
  ambient_color = vertex.color.rgb;
  diffuse_color = vertex.color.rgb;
  opacity = vertex.color.a;

  ///------------------------///
  // Representation: VTK_SURFACE with edge visibility turned on.
  ///------------------------///
  let representation = getRepresentation(actor.render_options.flags);
  let edge_visibility = getEdgeVisibility(actor.render_options.flags);
  if (representation == VTK_SURFACE && edge_visibility) {
    let use_line_width_for_edge_thickness = getUseLineWidthForEdgeThickness(actor.render_options.flags);
    let linewidth: f32 = select(actor.render_options.edge_width, actor.render_options.line_width, use_line_width_for_edge_thickness);
    // Undo perspective correction.
    let dist_vec = vertex.edge_dists.xyz * vertex.position.w;
    var d: f32 = 0.0;
    // Compute the shortest distance to the edge
    if vertex.hide_edge == 2.0 {
        d = min(dist_vec[0], dist_vec[2]);
    } else if vertex.hide_edge == 1.0 {
        d = dist_vec[0];
    } else if vertex.hide_edge == 0.0 {
        d = min(dist_vec[0], dist_vec[1]);
    } else { // no edge is hidden
        d = min(dist_vec[0], min(dist_vec[1], dist_vec[2]));
    }
    let half_linewidth: f32 = 0.5 * linewidth;
    let I: f32 = select(exp2(-2.0 * (d - half_linewidth) * (d - half_linewidth)), 1.0, d < half_linewidth);
    diffuse_color = mix(diffuse_color, actor.color_options.edge_color, I);
    ambient_color = mix(ambient_color, actor.color_options.edge_color, I);

    let render_lines_as_tubes = getRenderLinesAsTubes(actor.render_options.flags);
    if (render_lines_as_tubes) {
      if (d < 1.1 * half_linewidth) { // extend 10% to hide jagged artifacts on the edge-surface interface.
        normal_vc.z = 1.0 - (d / half_linewidth);
      }
    }
  }

  ///------------------------///
  // Normals
  ///------------------------///
  if !is_front_facing {
    if (normal_vc.z < 0.0) {
      normal_vc = -vertex.normal_vc;
      normal_vc = normalize(normal_vc);
    }
  } else if normal_vc.z < 0.0 {
    normal_vc.z = -normal_vc.z;
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
  return output;
}
