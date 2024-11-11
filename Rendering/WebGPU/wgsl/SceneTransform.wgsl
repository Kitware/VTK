// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
  inverted_projection: mat4x4<f32>
}
