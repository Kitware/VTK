// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

const BIT_MASK_PARALLEL_PROJECTION: u32 = 0x1; // mask to obtain lowest bit

//-------------------------------------------------------------------
struct SceneTransform {
  // origin and dimensions of view area.
  viewport: vec4<f32>,
  // world space -> camera space
  view: mat4x4<f32>,
  // camera space -> clip space
  projection: mat4x4<f32>,
  // inverse of the transpose of view matrix
  normal: mat3x3<f32>,
  // clip-space -> camera-space
  inverted_projection: mat4x4<f32>,
  // Integer/boolean options encoded inside flags. See below get methods.
  flags: u32
}

// Extracts bit that indicates whether orthographic projection is used. Returns true/false
fn getUseParallelProjection(flags: u32) -> bool
{
  let result: u32 = flags & BIT_MASK_PARALLEL_PROJECTION;
  return select(false, true, result == 1u);
}
