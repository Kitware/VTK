// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

//-------------------------------------------------------------------
struct ActorTransform {
  // model space -> world space
  world: mat4x4<f32>,
  // inverse of transposed world matrix
  normal: mat3x3<f32>
}
