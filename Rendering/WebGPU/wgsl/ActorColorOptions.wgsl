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
  // PBR metallic factor
  metallic: f32,
  // PBR roughness factor
  roughness: f32,
  // Reflectance at normal incidence of the base layer
  base_f0: f32,
  // PBR anisotropy strength
  anisotropy: f32,
  // Rotation of the anisotropy direction around the normal, in [0, 1]
  anisotropy_rotation: f32,
  // Clear coat layer strength
  coat_strength: f32,
  // Clear coat layer roughness
  coat_roughness: f32,
  // Reflectance at normal incidence of the clear coat layer
  coat_f0: f32,
  // Scale applied to the clear coat normal texture
  coat_normal_scale: f32,
  // Scale applied to the normal texture
  normal_scale: f32,
  // Emissive color factor
  emissive_factor: vec3<f32>,
  // Ambient occlusion strength
  occlusion_strength: f32,
  // Reflectance at grazing angle of metals
  edge_tint: vec3<f32>,
  // Clear coat layer color
  coat_color: vec3<f32>,
  // Color multiplying the reflectance at normal incidence of the dielectric base layer
  base_specular_color: vec3<f32>,
  // Weight of the specular reflection of the dielectric base layer
  base_specular_weight: f32,
}
