// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var<storage, read> connectivity: array<u32>;
@group(0) @binding(1) var<storage, read> offsets: array<u32>;
@group(0) @binding(2) var<storage, read> primitive_counts: array<u32>;
@group(0) @binding(3) var<uniform> cell_id_offset: u32;
@group(0) @binding(4) var<storage, read_write> out_connectivity: array<u32>;
@group(0) @binding(5) var<storage, read_write> cell_ids: array<u32>;
@group(0) @binding(6) var<storage, read_write> edge_array: array<f32>;

@compute @workgroup_size(64)
fn polygon_to_triangle(
    @builtin(workgroup_id) workgroup_id : vec3<u32>,
    @builtin(local_invocation_index) local_invocation_index: u32,
    @builtin(num_workgroups) num_workgroups: vec3<u32>)
{
  let workgroup_index =
    workgroup_id.x +
    workgroup_id.y * num_workgroups.x +
    workgroup_id.z * num_workgroups.x * num_workgroups.y;

  let cell_id: u32 =
    workgroup_index * 64u +
    local_invocation_index;
  if (cell_id >= arrayLength(&offsets) - 1u)
  {
    return;
  }

  let num_triangles_per_cell: u32 = primitive_counts[cell_id + 1u] - primitive_counts[cell_id];

  // where to start writing point indices of a triangle.
  var output_offset: u32 = primitive_counts[cell_id] * 3u;

  // where to obtain the point indices that describe connectivity of a polygon
  let input_offset: u32 = offsets[cell_id];

  for (var i: u32 = 0u; i < num_triangles_per_cell; i++)
  {
    let p0: u32 = connectivity[input_offset];
    let p1: u32 = connectivity[input_offset + i + 1u];
    let p2: u32 = connectivity[input_offset + i + 2u];

    let triangle_id: u32 = primitive_counts[cell_id] + i;
    // write edge array, this lets fragment shader hide internal edges of a polygon
    // when edge visibility is turned on.
    // edge_array[triangle_id] = (num_triangles_per_cell == 1) ? -1 : ((i == 0) ? 2 : ((i == num_triangles_per_cell - 1) ? 0 : 1));
    edge_array[triangle_id] = select(select(select(1f, 0f, (i == num_triangles_per_cell - 1)), 2f, (i == 0)), -1f, (num_triangles_per_cell == 1));
    cell_ids[triangle_id] = cell_id + cell_id_offset;
    out_connectivity[output_offset] = p0;
    output_offset++;
    out_connectivity[output_offset] = p1;
    output_offset++;
    out_connectivity[output_offset] = p2;
    output_offset++;
  }
}

@compute @workgroup_size(64)
fn poly_line_to_line(
    @builtin(workgroup_id) workgroup_id : vec3<u32>,
    @builtin(local_invocation_index) local_invocation_index: u32,
    @builtin(num_workgroups) num_workgroups: vec3<u32>)
{
  let workgroup_index =
    workgroup_id.x +
    workgroup_id.y * num_workgroups.x +
    workgroup_id.z * num_workgroups.x * num_workgroups.y;

  let cell_id: u32 =
    workgroup_index * 64u +
    local_invocation_index;
  if (cell_id >= arrayLength(&offsets) - 1u)
  {
    return;
  }

  let num_lines_per_cell: u32 = primitive_counts[cell_id + 1u] - primitive_counts[cell_id];

  // where to start writing point indices of a line.
  var output_offset: u32 = primitive_counts[cell_id] * 2u;

  // where to obtain the point indices that describe connectivity of a polyline
  let input_offset: u32 = offsets[cell_id];

  for (var i: u32 = 0u; i < num_lines_per_cell; i++)
  {
    let p0: u32 = connectivity[input_offset + i];
    let p1: u32 = connectivity[input_offset + i + 1u];

    let line_segment_id: u32 = primitive_counts[cell_id] + i;
    cell_ids[line_segment_id] = cell_id + cell_id_offset;
    out_connectivity[output_offset] = p0;
    output_offset++;
    out_connectivity[output_offset] = p1;
    output_offset++;
  }
}

@compute @workgroup_size(64)
fn poly_vertex_to_vertex(
    @builtin(workgroup_id) workgroup_id : vec3<u32>,
    @builtin(local_invocation_index) local_invocation_index: u32,
    @builtin(num_workgroups) num_workgroups: vec3<u32>)
{
  let workgroup_index =
    workgroup_id.x +
    workgroup_id.y * num_workgroups.x +
    workgroup_id.z * num_workgroups.x * num_workgroups.y;

  let cell_id: u32 =
    workgroup_index * 64u +
    local_invocation_index;
  if (cell_id >= arrayLength(&offsets) - 1u)
  {
    return;
  }

  let num_vertices_per_cell: u32 = primitive_counts[cell_id + 1u] - primitive_counts[cell_id];

  // where to start writing point index of a vertex.
  var output_offset: u32 = primitive_counts[cell_id];

  // where to obtain the point indices that describe connectivity of a polyvertex
  let input_offset: u32 = offsets[cell_id];

  for (var i: u32 = 0u; i < num_vertices_per_cell; i++)
  {
    let p0: u32 = connectivity[input_offset + i];

    let vertex_id: u32 = primitive_counts[cell_id] + i;
    cell_ids[vertex_id] = cell_id + cell_id_offset;
    out_connectivity[output_offset] = p0;
    output_offset++;
  }
}

@compute @workgroup_size(64)
fn polygon_edges_to_lines(
    @builtin(workgroup_id) workgroup_id : vec3<u32>,
    @builtin(local_invocation_index) local_invocation_index: u32,
    @builtin(num_workgroups) num_workgroups: vec3<u32>)
{
  let workgroup_index =
    workgroup_id.x +
    workgroup_id.y * num_workgroups.x +
    workgroup_id.z * num_workgroups.x * num_workgroups.y;

  let cell_id: u32 =
    workgroup_index * 64u +
    local_invocation_index;
  if (cell_id >= arrayLength(&offsets) - 1u)
  {
    return;
  }

  let num_lines_per_cell: u32 = primitive_counts[cell_id + 1u] - primitive_counts[cell_id];

  // where to start writing point indices of a line.
  var output_offset: u32 = primitive_counts[cell_id] * 2u;

  // where to obtain the point indices that describe connectivity of a polygon
  let input_offset: u32 = offsets[cell_id];

  for (var i: u32 = 0u; i < num_lines_per_cell; i++)
  {
    let p0: u32 = connectivity[input_offset + i];
    let p1: u32 = connectivity[input_offset + (i + 1u) % num_lines_per_cell];

    let line_segment_id = primitive_counts[cell_id] + i;
    cell_ids[line_segment_id] = cell_id + cell_id_offset;
    out_connectivity[output_offset] = p0;
    output_offset++;
    out_connectivity[output_offset] = p1;
    output_offset++;
  }
}

@compute @workgroup_size(64)
fn cell_to_points(
    @builtin(workgroup_id) workgroup_id : vec3<u32>,
    @builtin(local_invocation_index) local_invocation_index: u32,
    @builtin(num_workgroups) num_workgroups: vec3<u32>)
{
  let workgroup_index =
    workgroup_id.x +
    workgroup_id.y * num_workgroups.x +
    workgroup_id.z * num_workgroups.x * num_workgroups.y;

  let cell_id: u32 =
    workgroup_index * 64u +
    local_invocation_index;
  if (cell_id >= arrayLength(&offsets) - 1u)
  {
    return;
  }

  let num_vertices_per_cell: u32 = primitive_counts[cell_id + 1u] - primitive_counts[cell_id];

  // where to start writing point indices of a point.
  var output_offset: u32 = primitive_counts[cell_id];

  // where to obtain the point indices that describe connectivity of a polygon
  let input_offset: u32 = offsets[cell_id];

  for (var i: u32 = 0u; i < num_vertices_per_cell; i++)
  {
    let p: u32 = connectivity[input_offset + i];

    let vertex_id: u32 = primitive_counts[cell_id] + i;
    cell_ids[vertex_id] = cell_id + cell_id_offset;
    out_connectivity[output_offset] = p;
    output_offset++;
  }
}
