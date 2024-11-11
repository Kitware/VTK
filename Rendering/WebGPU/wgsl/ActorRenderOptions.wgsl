// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

const VTK_POINTS: u32 = 0;
const VTK_WIREFRAME: u32 = 1;
const VTK_SURFACE: u32 = 2;

const POINT_2D_ROUND: u32 = 0;
const POINT_2D_SQUARE: u32 = 1;

//-------------------------------------------------------------------
struct ActorRenderOptions {
  // One of VTK_POINTS, VTK_WIREFRAME or VTK_SURFACE.
  representation: u32,
  // Point size in pixels - applicable when points are visible.
  point_size: f32,
  // Line width in pixels - applicable when edges are visible.
  line_width: f32,
  // Edge visibility - applicable for representation = VTK_SURFACE
  edge_visibility: u32,
  // Render points as spheres - applicable when rendering points.
  render_points_as_spheres: u32,
  // Render lines as tubes - applicable when rendering lines.
  render_lines_as_tubes: u32,
  // 2D shape of points: either POINT_2D_ROUND or POINT_2D_SQUARE
  point_2d_shape: u32,
}
