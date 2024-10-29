// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

//-------------------------------------------------------------------
struct ActorColorOptions {
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
  // 0: Global shading - global color for all primitives.
  // 1: Smooth shading - point based colors interpolated for in-between fragments.
  // 2: Flat shading - cell based colors
  shading_type: u32,
  // What kind of directional vectors are available to use for lighting?
  directional_mask: u32,
  // Material ambient color
  ambient_color: vec3<f32>,
  // Material diffuse color
  diffuse_color: vec3<f32>,
  // Material specular color
  specular_color: vec3<f32>,
  // Edge color
  edge_color: vec3<f32>,
  // Vertex color
  vertex_color: vec3<f32>
}
