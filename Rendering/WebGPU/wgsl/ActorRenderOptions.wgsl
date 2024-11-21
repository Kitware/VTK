// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

const VTK_POINTS: u32 = 0;
const VTK_WIREFRAME: u32 = 1;
const VTK_SURFACE: u32 = 2;

const POINT_2D_ROUND: u32 = 0;
const POINT_2D_SQUARE: u32 = 1;

const BIT_MASK_REPRESENTATION: u32 = 0x3; // mask to obtain lower 2 bits
const BIT_POSITION_EDGE_VISIBILITY: u32 = 2u;
const BIT_POSITION_VERTEX_VISIBILITY: u32 = 3u;
const BIT_POSITION_USE_LINE_WIDTH_FOR_EDGE_THICKNESS: u32 = 4u;
const BIT_POSITION_RENDER_POINTS_AS_SPHERES: u32 = 5u;
const BIT_POSITION_RENDER_LINES_AS_TUBES: u32 = 6u;
const BIT_POSITION_POINT_2D_SHAPE: u32 = 7u;

//-------------------------------------------------------------------
struct ActorRenderOptions {
  // Point size in pixels - applicable when points are visible.
  point_size: f32,
  // Line width in pixels - applicable when edges are visible.
  line_width: f32,
  // Edge width in pixels - applicable when edges are visible and flags has the bit set for UseLineWidthForEdgeThickness
  edge_width: f32,
  // Integer/boolean options encoded inside flags. See below get methods.
  flags: u32
}

// Extracts representation from flags. Returns of VTK_POINTS, VTK_WIREFRAME or VTK_SURFACE.
fn getRepresentation(flags: u32) -> u32
{
  return flags & BIT_MASK_REPRESENTATION;
}

// Extracts edge visibility from flags. Returns true/false
fn getEdgeVisibility(flags: u32) -> bool
{
  let result: u32 = (flags >> BIT_POSITION_EDGE_VISIBILITY) & 0x1;
  return select(false, true, result == 1u);
}

// Extracts vertex visibility from flags. Returns true/false
fn getVertexVisibility(flags: u32) -> bool
{
  let result: u32 = (flags >> BIT_POSITION_VERTEX_VISIBILITY) & 0x1;
  return select(false, true, result == 1u);
}

// Extracts edge visibility from flags. Returns true/false
fn getUseLineWidthForEdgeThickness(flags: u32) -> bool
{
  let result: u32 = (flags >> BIT_POSITION_USE_LINE_WIDTH_FOR_EDGE_THICKNESS) & 0x1;
  return select(false, true, result == 1u);
}

// Extracts boolean that specifies whether points are rendered as spheres from flags. Returns true/false
fn getRenderPointsAsSpheres(flags: u32) -> bool
{
  let result: u32 = (flags >> BIT_POSITION_RENDER_POINTS_AS_SPHERES) & 0x1;
  return select(false, true, result == 1u);
}

// Extracts boolean that specifies whether lines are rendered as tubes from flags. Returns true/false
fn getRenderLinesAsTubes(flags: u32) -> bool
{
  let result: u32 = (flags >> BIT_POSITION_RENDER_LINES_AS_TUBES) & 0x1;
  return select(false, true, result == 1u);
}

// Extracts 2-dimensional shape of points from flags. Returns true/false
fn getPoint2DShape(flags: u32) -> u32
{
  let result: u32 = (flags >> BIT_POSITION_POINT_2D_SHAPE) & 0x1;
  return select(POINT_2D_ROUND, POINT_2D_SQUARE, result == 1u);
}
