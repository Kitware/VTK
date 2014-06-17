#version 330 core

//////////////////////////////////////////////////////////////////////////////
///
/// Inputs
///
//////////////////////////////////////////////////////////////////////////////

/// Object space vertex position
layout(location = 0) in vec3 m_in_vertex_pos;

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms
///
//////////////////////////////////////////////////////////////////////////////

/// combined modelview projection matrix
uniform mat4 m_modelview_matrix;
uniform mat4 m_projection_matrix;
uniform mat4 m_scene_matrix;

uniform vec3 m_vol_extents_min;
uniform vec3 m_vol_extents_max;

uniform vec3 m_texture_extents_max;
uniform vec3 m_texture_extents_min;

//////////////////////////////////////////////////////////////////////////////
///
/// Outputs
///
//////////////////////////////////////////////////////////////////////////////

/// 3D texture coordinates for texture lookup in the fragment shader
out vec3 m_texture_coords;
out vec3 m_vertex_pos;

void main()
{
  /// Get clipspace position
  mat4 ogl_projection_matrix = transpose(m_projection_matrix);
  mat4 ogl_modelview_matrix = transpose(m_modelview_matrix);
  vec4 pos = ogl_projection_matrix * ogl_modelview_matrix * transpose(m_scene_matrix) *
             vec4(m_in_vertex_pos.xyz, 1);
  gl_Position = pos;
  m_vertex_pos = m_in_vertex_pos;

  /// Compute texture coordinates
  /// Assuming point data only. Also, we offset the texture coordinate to account
  /// for OpenGL treating voxel at the center of the cell.
  vec3 uvx = (m_in_vertex_pos - m_vol_extents_min) / (m_vol_extents_max - m_vol_extents_min);
  vec3 delta = m_texture_extents_max - m_texture_extents_min;
  m_texture_coords = (uvx * (delta - vec3(1.0)) + vec3(0.5)) / delta;
}
