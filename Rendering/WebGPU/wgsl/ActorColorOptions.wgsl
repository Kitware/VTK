// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

const VTK_FLAT: u32 = 0;
const VTK_GOURAUD: u32 = 1;
const VTK_PHONG: u32 = 2;
const VTK_PBR: u32 = 3;

//-------------------------------------------------------------------
struct ActorColorOptions {
  // Material ambient color
  ambient_color: vec3<f32>,
  // Material diffuse color
  diffuse_color: vec3<f32>,
  // Material specular color
  specular_color: vec3<f32>,
  // Edge color
  edge_color: vec3<f32>,
  // Vertex color
  vertex_color: vec3<f32>,
  // Material ambient color intensity
  ambient_intensity: f32,
  // Material diffuse color intensity
  diffuse_intensity: f32,
  // Material specular color intensity
  specular_intensity: f32,
  // Material specular power
  specular_power: f32,
  // Opacity level
  opacity: f32,
  // One of VTK_FLAT, VTK_GOURAUD, VTK_PHONG and VTK_PBR
  interpolation_type: u32,
  // Id to color by
  id: u32,
}
