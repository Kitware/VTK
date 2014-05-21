#version 330 core

// Inputs
//
//////////////////////////////////////////////////////////////////////////////

/// Object space vertex position
layout(location = 0) in vec3 in_vertex_pos;

// Uniforms
//
//////////////////////////////////////////////////////////////////////////////

/// combined modelview projection matrix
uniform mat4 modelview_matrix;
uniform mat4 projection_matrix;
uniform mat4 scene_matrix;

uniform vec3 vol_extents_min;
uniform vec3 vol_extents_max;

uniform vec3 texture_extents_max;
uniform vec3 texture_extents_min;

// Outputs
//
//////////////////////////////////////////////////////////////////////////////

/// 3D texture coordinates for texture lookup in the fragment shader
out vec3 texture_coords;
out vec3 vertex_pos;

void main()
{
  /// Get clipspace position
  mat4 ogl_projection_matrix = transpose(projection_matrix);
  mat4 ogl_modelview_matrix = transpose(modelview_matrix);
  vec4 pos = ogl_projection_matrix * ogl_modelview_matrix * transpose(scene_matrix) *
             vec4(in_vertex_pos.xyz, 1);
  gl_Position = pos;
  vertex_pos = in_vertex_pos;

  /// Compute texture coordinates
  /// Assuming point data only. Also, we offset the texture coordinate to account
  /// for OpenGL treating voxel at the center of the cell.
  vec3 uvx = (in_vertex_pos - vol_extents_min) / (vol_extents_max - vol_extents_min);
  vec3 delta = texture_extents_max - texture_extents_min;
  texture_coords = (uvx * (delta - vec3(1.0)) + vec3(0.5)) / delta;
}
