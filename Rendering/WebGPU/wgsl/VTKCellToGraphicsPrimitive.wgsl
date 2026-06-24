// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var<storage, read> connectivity: array<u32>;
@group(0) @binding(1) var<storage, read> offsets: array<u32>;
@group(0) @binding(2) var<storage, read> primitive_counts: array<u32>;
@group(0) @binding(3) var<uniform> cell_id_offset: u32;
@group(0) @binding(4) var<storage, read_write> out_connectivity: array<u32>;
@group(0) @binding(5) var<storage, read_write> cell_ids: array<u32>;
@group(0) @binding(6) var<storage, read_write> edge_array: array<f32>;
// Point coordinates (flat xyz, indexed by point id) used by polygon_to_triangle
// to ear-clip non-convex polygons. Only the polygon_to_triangle entry point
// declares/uses this binding.
@group(0) @binding(7) var<storage, read> point_coordinates: array<f32>;

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
  // where to obtain the point indices that describe connectivity of a polygon.
  let input_offset: u32 = offsets[cell_id];
  // number of vertices in this polygon (== num_triangles_per_cell + 2 for a simple polygon).
  let cell_size: u32 = num_triangles_per_cell + 2u;

  // Fixed-capacity ring for ear clipping. Iso-polygons and rendered polygons are
  // small; MAX_POLY=32 is generous. Polygons larger than the cap fall back to a
  // fan (correct for convex, acceptable degeneracy otherwise).
  const MAX_POLY: u32 = 32u;

  // Triangles and quads need no ear clipping: a triangle is itself, and any
  // simple quad is correctly split by a diagonal from vertex 0. For these we keep
  // the original fan emission and edge-flag convention so behavior is unchanged.
  if (cell_size <= 4u || cell_size > MAX_POLY)
  {
    for (var i: u32 = 0u; i < num_triangles_per_cell; i++)
    {
      let p0: u32 = connectivity[input_offset];
      let p1: u32 = connectivity[input_offset + i + 1u];
      let p2: u32 = connectivity[input_offset + i + 2u];
      let triangle_id: u32 = primitive_counts[cell_id] + i;
      // Boundary-edge mask (bit0: p0-p1, bit1: p1-p2, bit2: p2-p0). For a fan from
      // vertex 0, edge p1-p2 is always a polygon boundary edge; edge p0-p1 is a
      // boundary edge only for the first triangle and p2-p0 only for the last.
      var mask: u32 = 2u; // p1-p2 always boundary
      if (i == 0u) { mask = mask | 1u; }
      if (i == num_triangles_per_cell - 1u) { mask = mask | 4u; }
      edge_array[triangle_id] = f32(mask);
      cell_ids[triangle_id] = cell_id + cell_id_offset;
      out_connectivity[output_offset] = p0; output_offset++;
      out_connectivity[output_offset] = p1; output_offset++;
      out_connectivity[output_offset] = p2; output_offset++;
    }
    return;
  }

  // ---- Ear clipping for polygons with 5..MAX_POLY vertices ----
  // Build a compacted ring of LOCAL indices, dropping vertices coincident with
  // their predecessor (and the wrap-around duplicate). Banded contouring and
  // clipping can emit polygons with coincident consecutive vertices; the ear
  // test computes a bogus normal at a duplicated vertex, so they must be removed
  // first. This mirrors CompactPolygonRing in vtkOpenGLIndexBufferObject so both
  // renderers triangulate identically. ring[k] is the k-th distinct local index.
  var ring: array<u32, 32>;
  var m: u32 = 0u;
  for (var i: u32 = 0u; i < cell_size; i++)
  {
    let pid_i: u32 = connectivity[input_offset + i];
    let pi = vec3<f32>(point_coordinates[3u*pid_i], point_coordinates[3u*pid_i+1u], point_coordinates[3u*pid_i+2u]);
    var keep: bool = true;
    if (m > 0u)
    {
      let pid_prev_kept: u32 = connectivity[input_offset + ring[m - 1u]];
      let pk = vec3<f32>(point_coordinates[3u*pid_prev_kept], point_coordinates[3u*pid_prev_kept+1u], point_coordinates[3u*pid_prev_kept+2u]);
      if (pi.x == pk.x && pi.y == pk.y && pi.z == pk.z) { keep = false; }
    }
    if (keep) { ring[m] = i; m = m + 1u; }
  }
  // Drop last if coincident with first (wrap-around duplicate).
  if (m >= 2u)
  {
    let pid_first: u32 = connectivity[input_offset + ring[0]];
    let pid_last: u32 = connectivity[input_offset + ring[m - 1u]];
    let pf = vec3<f32>(point_coordinates[3u*pid_first], point_coordinates[3u*pid_first+1u], point_coordinates[3u*pid_first+2u]);
    let pl = vec3<f32>(point_coordinates[3u*pid_last], point_coordinates[3u*pid_last+1u], point_coordinates[3u*pid_last+2u]);
    if (pf.x == pl.x && pf.y == pl.y && pf.z == pl.z) { m = m - 1u; }
  }

  // The output triangle count is fixed at cell_size - 2 by the CPU-side
  // primitive_counts. After compaction the ear clip produces only m - 2 real
  // triangles, so (cell_size - m) zero-area filler triangles are appended to
  // keep the output buffer fully written (they rasterize to nothing). A fully
  // degenerate polygon (m < 3) emits all fillers.
  var tris_emitted: u32 = 0u;
  let total_triangles: u32 = num_triangles_per_cell; // == cell_size - 2

  if (m >= 3u)
  {
    // Doubly linked ring over COMPACTED positions 0..m-1.
    var nxt: array<u32, 32>;
    var prv: array<u32, 32>;
    for (var i: u32 = 0u; i < m; i++)
    {
      nxt[i] = (i + 1u) % m;
      prv[i] = (i + m - 1u) % m;
    }

    // Newell normal over the compacted ring.
    var normal = vec3<f32>(0.0, 0.0, 0.0);
    {
      var pid_prev: u32 = connectivity[input_offset + ring[m - 1u]];
      var pp = vec3<f32>(point_coordinates[3u*pid_prev], point_coordinates[3u*pid_prev+1u], point_coordinates[3u*pid_prev+2u]);
      for (var i: u32 = 0u; i < m; i++)
      {
        let pid: u32 = connectivity[input_offset + ring[i]];
        let p = vec3<f32>(point_coordinates[3u*pid], point_coordinates[3u*pid+1u], point_coordinates[3u*pid+2u]);
        normal.x += (pp.y - p.y) * (pp.z + p.z);
        normal.y += (pp.z - p.z) * (pp.x + p.x);
        normal.z += (pp.x - p.x) * (pp.y + p.y);
        pp = p;
      }
    }

    // Ear clip walking forward from compacted vertex 1, advancing to the
    // clipped vertex's successor. For a convex polygon this clips 1,2,3,... in
    // order while vertex 0 stays the apex, reproducing exactly the fan-from-
    // vertex-0 triangulation (matching vtkOpenGLIndexBufferObject and the
    // historical renderer). Triangulation is not invariant under the choice of
    // interior diagonals: per-triangle texture-coordinate and Gouraud
    // interpolation and the edge mask depend on it, so convex polygons must keep
    // producing the fan.
    var remaining: u32 = m;
    var cur: u32 = 1u % m;
    var guard: u32 = 0u;
    let guard_max: u32 = 2u * m;

    while (remaining > 3u && guard < guard_max)
    {
      let a: u32 = prv[cur];
      let b: u32 = cur;
      let c: u32 = nxt[cur];
      let pa_id: u32 = connectivity[input_offset + ring[a]];
      let pb_id: u32 = connectivity[input_offset + ring[b]];
      let pc_id: u32 = connectivity[input_offset + ring[c]];
      let pa = vec3<f32>(point_coordinates[3u*pa_id], point_coordinates[3u*pa_id+1u], point_coordinates[3u*pa_id+2u]);
      let pb = vec3<f32>(point_coordinates[3u*pb_id], point_coordinates[3u*pb_id+1u], point_coordinates[3u*pb_id+2u]);
      let pc = vec3<f32>(point_coordinates[3u*pc_id], point_coordinates[3u*pc_id+1u], point_coordinates[3u*pc_id+2u]);

      // convexity at b: ((b-a) x (c-b)) . normal > 0
      let cross_b = cross(pb - pa, pc - pb);
      var is_ear: bool = dot(cross_b, normal) > 0.0;

      if (is_ear)
      {
        // no other remaining vertex may lie strictly inside triangle (a,b,c)
        var q: u32 = nxt[c];
        loop {
          if (q == a) { break; }
          let pq_id: u32 = connectivity[input_offset + ring[q]];
          let pq = vec3<f32>(point_coordinates[3u*pq_id], point_coordinates[3u*pq_id+1u], point_coordinates[3u*pq_id+2u]);
          let s1 = dot(cross(pb - pa, pq - pa), normal);
          let s2 = dot(cross(pc - pb, pq - pb), normal);
          let s3 = dot(cross(pa - pc, pq - pc), normal);
          if (s1 > 0.0 && s2 > 0.0 && s3 > 0.0)
          {
            is_ear = false;
            break;
          }
          q = nxt[q];
        }
      }

      if (is_ear)
      {
        let triangle_id: u32 = primitive_counts[cell_id] + tris_emitted;
        // boundary-edge mask: an edge is a polygon boundary iff its two
        // compacted-ring indices are adjacent modulo m.
        var mask: u32 = 0u;
        let dab: u32 = (b + m - a) % m;
        let dbc: u32 = (c + m - b) % m;
        let dca: u32 = (a + m - c) % m;
        if (dab == 1u || dab == m - 1u) { mask = mask | 1u; }
        if (dbc == 1u || dbc == m - 1u) { mask = mask | 2u; }
        if (dca == 1u || dca == m - 1u) { mask = mask | 4u; }
        edge_array[triangle_id] = f32(mask);
        cell_ids[triangle_id] = cell_id + cell_id_offset;
        out_connectivity[output_offset] = pa_id; output_offset++;
        out_connectivity[output_offset] = pb_id; output_offset++;
        out_connectivity[output_offset] = pc_id; output_offset++;
        tris_emitted++;
        // clip ear: remove b, advance to its successor (keeps vertex 0 as apex
        // for a convex polygon, reproducing the fan).
        nxt[a] = c;
        prv[c] = a;
        cur = c;
        remaining--;
        guard = 0u;
      }
      else
      {
        cur = nxt[cur];
        guard++;
      }
    }

    // final triangle, anchored at prv[cur] so a convex polygon emits
    // (0, m-2, m-1): the same vertex order and edge mask the fan emits.
    if (remaining == 3u)
    {
      let b: u32 = cur;
      let a: u32 = prv[cur];
      let c: u32 = nxt[cur];
      let pa_id: u32 = connectivity[input_offset + ring[a]];
      let pb_id: u32 = connectivity[input_offset + ring[b]];
      let pc_id: u32 = connectivity[input_offset + ring[c]];
      let triangle_id: u32 = primitive_counts[cell_id] + tris_emitted;
      var mask: u32 = 0u;
      let dab: u32 = (b + m - a) % m;
      let dbc: u32 = (c + m - b) % m;
      let dca: u32 = (a + m - c) % m;
      if (dab == 1u || dab == m - 1u) { mask = mask | 1u; }
      if (dbc == 1u || dbc == m - 1u) { mask = mask | 2u; }
      if (dca == 1u || dca == m - 1u) { mask = mask | 4u; }
      edge_array[triangle_id] = f32(mask);
      cell_ids[triangle_id] = cell_id + cell_id_offset;
      out_connectivity[output_offset] = pa_id; output_offset++;
      out_connectivity[output_offset] = pb_id; output_offset++;
      out_connectivity[output_offset] = pc_id; output_offset++;
      tris_emitted++;
    }
  }

  // Append zero-area filler triangles so exactly total_triangles are written.
  // Each uses the first connectivity point id three times: degenerate, so the
  // rasterizer produces no fragments. Edge mask 0 (no boundary edges).
  let filler_pid: u32 = connectivity[input_offset];
  while (tris_emitted < total_triangles)
  {
    let triangle_id: u32 = primitive_counts[cell_id] + tris_emitted;
    edge_array[triangle_id] = 0f;
    cell_ids[triangle_id] = cell_id + cell_id_offset;
    out_connectivity[output_offset] = filler_pid; output_offset++;
    out_connectivity[output_offset] = filler_pid; output_offset++;
    out_connectivity[output_offset] = filler_pid; output_offset++;
    tris_emitted++;
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
