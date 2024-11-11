// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

const VTK_LIGHT_TYPE_DEFERRED: u32 = 0;
const VTK_LIGHT_TYPE_HEADLIGHT: u32 = 1;
const VTK_LIGHT_TYPE_CAMERA_LIGHT: u32 = 2;
const VTK_LIGHT_TYPE_SCENE_LIGHT: u32 = 3;

//-------------------------------------------------------------------
struct SceneLight {
  light_type: u32, // See VTK_LIGHT_TYPE_XYZ
  positional: u32, // TODO: make it a boolean
  cone_angle: f32,
  exponent: f32,
  color: vec3<f32>,
  direction_vc: vec3<f32>,
  position_vc: vec3<f32>,
  attenuation: vec3<f32>
}

//-------------------------------------------------------------------
struct SceneLights {
  count: u32,
  values: array<SceneLight>
}
