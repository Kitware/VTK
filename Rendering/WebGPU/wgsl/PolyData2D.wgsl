// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// #include "VTK/wgsl/Utilities.wgsl"

//-------------------------------------------------------------------
struct Mapper2DState
{
  wcvc_matrix: mat4x4<f32>,
  color: vec4<f32>,
  point_size: f32,
  line_width: f32,
  flags: u32,
}

//-------------------------------------------------------------------
const BIT_POSITION_USE_CELL_COLOR: u32 = 0u;
const BIT_POSITION_USE_POINT_COLOR: u32 = 1u;

//-------------------------------------------------------------------
fn getUseCellColor(flags: u32) -> bool
{
  let result: u32 = (flags >> BIT_POSITION_USE_CELL_COLOR) & 0x1;
  return select(false, true, result == 1u);
}

//-------------------------------------------------------------------
fn getUsePointColor(flags: u32) -> bool
{
  let result: u32 = (flags >> BIT_POSITION_USE_POINT_COLOR) & 0x1;
  return select(false, true, result == 1u);
}

//-------------------------------------------------------------------
struct F32s {
  values: array<f32>
}

//-------------------------------------------------------------------
struct MeshAttributeArrayDescriptor {
  start: u32,
  num_tuples: u32,
  num_components: u32
}

//-------------------------------------------------------------------
struct MeshDescriptor {
  position: MeshAttributeArrayDescriptor,
  point_uv: MeshAttributeArrayDescriptor,
  colors: MeshAttributeArrayDescriptor,
}

//-------------------------------------------------------------------
struct Topology {
  // the vtk cell ID for this index. used to index into a cell attribute.
  cell_id: u32,
  // the vtk point ID for this index. used to index into a point attribute.
  point_id: u32,
}

///-----------------------------------------------------------------///
// Mesh attributes.
///-----------------------------------------------------------------///
@group(0) @binding(0) var<storage, read> state: Mapper2DState;
@group(0) @binding(1) var<storage, read> mesh_descriptor: MeshDescriptor;
@group(0) @binding(2) var<storage, read> mesh_data: F32s;

///-----------------------------------------------------------------///
// Topology
///-----------------------------------------------------------------///
@group(1) @binding(0) var<storage, read> topology: array<Topology>;

//-------------------------------------------------------------------
struct VertexInput {
  @builtin(instance_index) instance_id: u32,
  @builtin(vertex_index) vertex_id: u32
}

//-------------------------------------------------------------------
struct VertexOutput {
  @builtin(position) position: vec4<f32>,
  @location(0) color: vec4<f32>,
  @location(1) uv: vec2<f32>,
  @location(2) @interpolate(flat) cell_id: u32,
}

//-------------------------------------------------------------------
@vertex
fn pointVertexMain(vertex: VertexInput) -> VertexOutput {
  var output: VertexOutput;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.instance_id;
  // get CellID from vertex ID -> VTK cell map.
  output.cell_id = topology[pull_vertex_id].cell_id;
  // pull the point id
  let point_id = topology[pull_vertex_id].point_id;
  // pull the position for this vertex.
  var vertex_wc = vec4<f32>(getTuple3F32(point_id, mesh_descriptor.position.start, &mesh_data.values), 1.0);

  var point_size = state.point_size;
  // The point rendering algorithm is unstable for point_size < 1.0
  if point_size < 1.0 {
    point_size = 1.0;
  }
  ///
  // (-1, 1) |-------------------------------|(1, 1)
  //         |-                              |
  //         |    -                          |
  //         |        -                      |
  // (-1, 0) |              -                |
  //         |                   -           |
  //         |                        -      |
  //         |                              -|
  // (-1,-1) |-------------------------------|(1, -1)
  ///
  // this triangle strip describes a quad spanning a bi-unit domain.
  const TRIANGLE_VERTS = array(
    vec2f(-1, -1),
    vec2f(1, -1),
    vec2f(-1, 1),
    vec2f(1, 1),
  );
  let local_position = TRIANGLE_VERTS[vertex.vertex_id];
  vertex_wc = vec4f(vertex_wc.xy + 0.5 * point_size * local_position, vertex_wc.zw);
  output.position = state.wcvc_matrix * vertex_wc;

  ///------------------------///
  // Texture coordinates
  ///------------------------///
  output.uv = getTuple2F32(point_id, mesh_descriptor.point_uv.start, &mesh_data.values);

  ///------------------------///
  // Smooth/Flat shading
  ///------------------------///
  if getUsePointColor(state.flags) {
    // Smooth shading
    output.color = getTuple4F32(point_id, mesh_descriptor.colors.start, &mesh_data.values);
  } else if getUseCellColor(state.flags) {
    // Flat shading
    output.color = getTuple4F32(output.cell_id, mesh_descriptor.colors.start, &mesh_data.values);
  } else {
    output.color = state.color;
  }

  return output;
}

//-------------------------------------------------------------------
@vertex
fn lineVertexMain(vertex: VertexInput) -> VertexOutput {
  var output: VertexOutput;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.instance_id;
  // get CellID from vertex ID -> VTK cell map.
  output.cell_id = topology[pull_vertex_id].cell_id;

  var width = state.line_width;
  // The point rendering algorithm is unstable for line_width < 1.0
  if width < 1.0 {
    width = 1.0;
  }

  ///
  // (0, 0.5) |-------------------------------|(1, 0.5)
  //          |-                              |
  //          |    -                          |
  //          |        -                      |
  // (0, 0)   |              -                |
  //          |                   -           |
  //          |                        -      |
  //          |                              -|
  // (0,-0.5) |-------------------------------|(1, -0.5)
  ///
  const TRIANGLE_VERTS = array(
    vec2(0, -0.5),
    vec2(1, -0.5),
    vec2(0, 0.5),
    vec2(1, 0.5),
  );
  let line_segment_id: u32 = vertex.instance_id;
  let local_position = TRIANGLE_VERTS[vertex.vertex_id];
  let p0_vertex_id: u32 = 2 * line_segment_id;
  let p1_vertex_id = p0_vertex_id + 1;

  let p0_point_id: u32 = topology[p0_vertex_id].point_id;
  let p1_point_id: u32 = topology[p1_vertex_id].point_id;
  let p = select(2 * line_segment_id, 2 * line_segment_id + 1, local_position.x == 1);

  let p0_vertex_wc = vec4<f32>(getTuple3F32(p0_point_id, mesh_descriptor.position.start, &mesh_data.values), 1.0);
  let p1_vertex_wc = vec4<f32>(getTuple3F32(p1_point_id, mesh_descriptor.position.start, &mesh_data.values), 1.0);

  let x_basis = normalize(p1_vertex_wc.xy - p1_vertex_wc.xy);
  let y_basis = vec2(-x_basis.y, x_basis.x);

  var vertex_wc = mix(p0_vertex_wc, p1_vertex_wc, local_position.x);
  vertex_wc = vec4(vertex_wc.x, vertex_wc.y + local_position.y * width, vertex_wc.zw);

  output.position = state.wcvc_matrix * vertex_wc;

  // mix point id
  let point_id = u32(mix(f32(p0_point_id), f32(p1_point_id), local_position.x));

  ///------------------------///
  // Texture coordinates
  ///------------------------///
  output.uv = getTuple2F32(point_id, mesh_descriptor.point_uv.start, &mesh_data.values);

  ///------------------------///
  // Smooth/Flat shading
  ///------------------------///
  if getUsePointColor(state.flags) {
    // Smooth shading
    output.color = getTuple4F32(point_id, mesh_descriptor.colors.start, &mesh_data.values);
  } else if getUseCellColor(state.flags) {
    // Flat shading
    output.color = getTuple4F32(output.cell_id, mesh_descriptor.colors.start, &mesh_data.values);
  } else {
    output.color = state.color;
  }

  return output;
}

//-------------------------------------------------------------------
@vertex
fn polygonVertexMain(vertex: VertexInput) -> VertexOutput {
  var output: VertexOutput;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.vertex_id;
  // get CellID from vertex ID -> VTK cell map.
  output.cell_id = topology[pull_vertex_id].cell_id;
  // pull the point id
  let point_id = topology[pull_vertex_id].point_id;
  // pull the position for this vertex.
  let vertex_wc = vec4<f32>(getTuple3F32(point_id, mesh_descriptor.position.start, &mesh_data.values), 1.0);
  output.position = state.wcvc_matrix * vertex_wc;

  ///------------------------///
  // Texture coordinates
  ///------------------------///
  output.uv = getTuple2F32(point_id, mesh_descriptor.point_uv.start, &mesh_data.values);

  ///------------------------///
  // Smooth/Flat shading
  ///------------------------///
  if getUsePointColor(state.flags) {
    // Smooth shading
    output.color = getTuple4F32(point_id, mesh_descriptor.colors.start, &mesh_data.values);
  } else if getUseCellColor(state.flags) {
    // Flat shading
    output.color = getTuple4F32(output.cell_id, mesh_descriptor.colors.start, &mesh_data.values);
  } else {
    output.color = state.color;
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
fn fragmentMain(vertex: VertexOutput) -> FragmentOutput {
  var output: FragmentOutput;
  output.color = vertex.color;
  output.ids.x = vertex.cell_id;
  return output;
}
