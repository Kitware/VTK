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
struct F32s
{
  values: array<f32>
}

//-------------------------------------------------------------------
struct MeshAttributeArrayDescriptor
{
  start: u32,
  num_tuples: u32,
  num_components: u32
}

//-------------------------------------------------------------------
struct MeshDescriptor
{
  position: MeshAttributeArrayDescriptor,
  point_uv: MeshAttributeArrayDescriptor,
  colors: MeshAttributeArrayDescriptor,
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
@group(1) @binding(0) var<storage, read> connectivity: array<u32>;
@group(1) @binding(1) var<storage, read> cell_ids: array<u32>;
@group(1) @binding(2) var<uniform> cell_id_offset: u32;

//-------------------------------------------------------------------
struct VertexInput
{
  @builtin(instance_index) instance_id: u32,
  @builtin(vertex_index) vertex_id: u32
}

//-------------------------------------------------------------------
struct VertexOutput
{
  @builtin(position) position: vec4<f32>,
  @location(0) color: vec4<f32>,
  @location(1) uv: vec2<f32>,
  @location(2) @interpolate(flat) cell_id: u32,
}

//-------------------------------------------------------------------
fn getVertexCoordinates(point_id: u32) -> vec4f
{
  return vec4f(mesh_data.values[mesh_descriptor.position.start + 3u * point_id], mesh_data.values[mesh_descriptor.position.start + 3u * point_id + 1u], mesh_data.values[mesh_descriptor.position.start + 3u * point_id + 2u], 1.0);
}

//-------------------------------------------------------------------
fn getVertexUVs(point_id: u32) -> vec2f
{
  return vec2f(mesh_data.values[mesh_descriptor.point_uv.start + 2u * point_id], mesh_data.values[mesh_descriptor.point_uv.start + 2u * point_id + 1u]);
}

//-------------------------------------------------------------------
fn getVertexColors(point_id: u32, cell_id: u32) -> vec4f
{
  if getUsePointColor(state.flags)
  {
    // Smooth shading
    return vec4f(mesh_data.values[mesh_descriptor.colors.start + 4u * point_id], mesh_data.values[mesh_descriptor.colors.start + 4u * point_id + 1u], mesh_data.values[mesh_descriptor.colors.start + 4u * point_id + 2u], mesh_data.values[mesh_descriptor.colors.start + 4u * point_id + 3u]);
  }
  if getUseCellColor(state.flags)
  {
    // Flat shading
    return vec4f(mesh_data.values[mesh_descriptor.colors.start + 4u * cell_id], mesh_data.values[mesh_descriptor.colors.start + 4u * cell_id + 1u], mesh_data.values[mesh_descriptor.colors.start + 4u * cell_id + 2u], mesh_data.values[mesh_descriptor.colors.start + 4u * cell_id + 3u]);
  }
  return state.color;
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
const VERTEX_PARAMETRIC_COORDS = array(
  vec2f(-1, -1),
  vec2f(1, -1),
  vec2f(-1, 1),
  vec2f(1, 1),
);

//-------------------------------------------------------------------
@vertex
fn pointVertexMain(vertex: VertexInput) -> VertexOutput
{
  var output: VertexOutput;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.instance_id;
  let primitive_id = pull_vertex_id;
  // get CellID from primitive ID -> VTK cell map.
  let cell_id = cell_ids[primitive_id];
  // pull the point id
  let point_id = connectivity[pull_vertex_id];
  // pull the position for this vertex.
  var vertex_wc = getVertexCoordinates(point_id);

  var point_size = state.point_size;
  // The point rendering algorithm is unstable for point_size < 1.0
  if point_size < 1.0
  {
    point_size = 1.0;
  }

  let local_position = VERTEX_PARAMETRIC_COORDS[vertex.vertex_id];
  vertex_wc = vec4f(vertex_wc.xy + 0.5 * point_size * local_position, vertex_wc.zw);

  output.position = state.wcvc_matrix * vertex_wc;
  output.cell_id = cell_id;
  output.uv = getVertexUVs(point_id);
  output.color = getVertexColors(point_id, cell_id);
  return output;
}

//-------------------------------------------------------------------
@vertex
fn pointVertexMainHomogeneousCellSize(vertex: VertexInput) -> VertexOutput
{
  var output: VertexOutput;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.instance_id;
  let primitive_id = pull_vertex_id;
  let cell_id = primitive_id + cell_id_offset;
  // pull the point id
  let point_id = connectivity[pull_vertex_id];
  // pull the position for this vertex.
  var vertex_wc = getVertexCoordinates(point_id);

  var point_size = state.point_size;
  // The point rendering algorithm is unstable for point_size < 1.0
  if point_size < 1.0
  {
    point_size = 1.0;
  }
  let local_position = VERTEX_PARAMETRIC_COORDS[vertex.vertex_id];
  vertex_wc = vec4f(vertex_wc.xy + 0.5 * point_size * local_position, vertex_wc.zw);

  output.position = state.wcvc_matrix * vertex_wc;
  output.cell_id = cell_id;
  output.uv = getVertexUVs(point_id);
  output.color = getVertexColors(point_id, cell_id);
  return output;
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
const LINE_PARAMETRIC_COORDS = array(
  vec2(0, -0.5),
  vec2(1, -0.5),
  vec2(0, 0.5),
  vec2(1, 0.5),
);

//-------------------------------------------------------------------
fn getLinePointWorldCoordinate(line_segment_id: u32, parametric_id: u32, out_point_id: ptr<function, u32>) -> vec4f
{
  var width = state.line_width;
  // The point rendering algorithm is unstable for line_width < 1.0
  if width < 1.0
  {
    width = 1.0;
  }

  let local_position = LINE_PARAMETRIC_COORDS[parametric_id];
  let p0_vertex_id: u32 = 2 * line_segment_id;
  let p1_vertex_id = p0_vertex_id + 1;

  let p0_point_id: u32 = connectivity[p0_vertex_id];
  let p1_point_id: u32 = connectivity[p1_vertex_id];
  let p = select(2 * line_segment_id, 2 * line_segment_id + 1, local_position.x == 1);
  // compute point id based on the x component of the parametric coordinate.
  *out_point_id = u32(mix(f32(p0_point_id), f32(p1_point_id), local_position.x));

  let p0_vertex_wc = getVertexCoordinates(p0_point_id);
  let p1_vertex_wc = getVertexCoordinates(p1_point_id);

  let x_basis = normalize(p1_vertex_wc.xy - p1_vertex_wc.xy);
  let y_basis = vec2(-x_basis.y, x_basis.x);

  var vertex_wc = mix(p0_vertex_wc, p1_vertex_wc, local_position.x);
  return vec4(vertex_wc.x, vertex_wc.y + local_position.y * width, vertex_wc.zw);
}

//-------------------------------------------------------------------
@vertex
fn lineVertexMain(vertex: VertexInput) -> VertexOutput
{
  var output: VertexOutput;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.instance_id;
  let primitive_id = pull_vertex_id / 2u;
  // get CellID from primitive ID -> VTK cell map.
  let cell_id = cell_ids[primitive_id];

  let line_segment_id = vertex.instance_id;
  let parametric_id = vertex.vertex_id;

  var point_id: u32;
  let vertex_wc = getLinePointWorldCoordinate(line_segment_id, parametric_id, &point_id);

  output.position = state.wcvc_matrix * vertex_wc;
  output.cell_id = cell_id;
  output.uv = getVertexUVs(point_id);
  output.color = getVertexColors(point_id, cell_id);
  return output;
}

//-------------------------------------------------------------------
@vertex
fn lineVertexMainHomogeneousCellSize(vertex: VertexInput) -> VertexOutput
{
  var output: VertexOutput;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.instance_id;
  let primitive_id = pull_vertex_id / 2u;
  let cell_id = primitive_id + cell_id_offset;

  let line_segment_id = vertex.instance_id;
  let parametric_id = vertex.vertex_id;

  var point_id: u32;
  let vertex_wc = getLinePointWorldCoordinate(line_segment_id, parametric_id, &point_id);

  output.position = state.wcvc_matrix * vertex_wc;
  output.cell_id = cell_id;
  output.uv = getVertexUVs(point_id);
  output.color = getVertexColors(point_id, cell_id);
  return output;
}

//-------------------------------------------------------------------
@vertex
fn polygonVertexMain(vertex: VertexInput) -> VertexOutput
{
  var output: VertexOutput;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.vertex_id;
  let primitive_id = pull_vertex_id / 3u;
  // get CellID from primitive ID -> VTK cell map.
  let cell_id = cell_ids[primitive_id];
  // pull the point id
  let point_id = connectivity[pull_vertex_id];
  // pull the position for this vertex.
  let vertex_wc = vec4f(mesh_data.values[mesh_descriptor.position.start + 3u * point_id], mesh_data.values[mesh_descriptor.position.start + 3u * point_id + 1u], mesh_data.values[mesh_descriptor.position.start + 3u * point_id + 2u], 1.0);

  output.position = state.wcvc_matrix * vertex_wc;
  output.cell_id = cell_id;
  output.uv = getVertexUVs(point_id);
  output.color = getVertexColors(point_id, cell_id);
  return output;
}

//-------------------------------------------------------------------
@vertex
fn polygonVertexMainHomogeneousCellSize(vertex: VertexInput) -> VertexOutput
{
  var output: VertexOutput;

  ///------------------------///
  // Pull vertex quantities
  ///------------------------///
  let pull_vertex_id: u32 = vertex.vertex_id;
  let primitive_id = pull_vertex_id / 3u;
  // get CellID from primitive ID -> VTK cell map.
  let cell_id = primitive_id + cell_id_offset;
  // pull the point id
  let point_id = connectivity[pull_vertex_id];
  // pull the position for this vertex.
  let vertex_wc = getVertexCoordinates(point_id);

  output.position = state.wcvc_matrix * vertex_wc;
  output.cell_id = cell_id;
  output.uv = getVertexUVs(point_id);
  output.color = getVertexColors(point_id, cell_id);
  return output;
}

//-------------------------------------------------------------------
struct FragmentOutput
{
  @location(0) color: vec4<f32>,
  @location(1) ids: vec4<u32>, // cell_id, prop_id, composite_id, process_id
}

//-------------------------------------------------------------------
@fragment
fn fragmentMain(vertex: VertexOutput) -> FragmentOutput
{
  var output: FragmentOutput;
  output.color = vertex.color;
  output.ids.x = vertex.cell_id;
  return output;
}
