#version 330 core

//////////////////////////////////////////////////////////////////////////////
///
/// Inputs
///
//////////////////////////////////////////////////////////////////////////////

/// 3D texture coordinates form vertex shader
in vec3 m_texture_coords;
in vec3 m_vertex_pos;

//////////////////////////////////////////////////////////////////////////////
///
/// Outputs
///
//////////////////////////////////////////////////////////////////////////////

/// Fragment shader output
layout(location = 0) out vec4 m_frag_color;

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms, attributes, and globals
///
//////////////////////////////////////////////////////////////////////////////
@BASE_GLOBALS_FRAG@
@TERMINATION_GLOBALS_FRAG@
@CROPPING_GLOBALS_FRAG@
@SHADING_GLOBALS_FRAG@

/// We support only 8 clipping planes for now
/// The first value is the size of the data array for clipping
/// planes (origin, normal)
uniform float m_clipping_planes[49];

//////////////////////////////////////////////////////////////////////////////
///
/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
{
  /// Initialize m_frag_color (output) to 0
  m_frag_color = vec4(0.0);

  @BASE_INIT@
  @TERMINATE_INIT@
  @SHADING_INIT@
  @CROPPING_INIT@

  float clipping_planes_ts[49];
  int clipping_planes_size = int(m_clipping_planes[0]);

  mat4 world_to_texture_mat = inverse(transpose(m_texture_dataset_matrix)) *
                              l_ogl_scene_matrix;
  for (int i = 0; i < clipping_planes_size / 6; ++i)
    {
    vec4 origin = vec4(m_clipping_planes[i + 1],
                       m_clipping_planes[i + 2],
                       m_clipping_planes[i + 3], 1.0);
    vec4 normal = vec4(m_clipping_planes[i + 4],
                       m_clipping_planes[i + 5],
                       m_clipping_planes[i + 6], 0.0);

    origin = world_to_texture_mat * origin;
    normal = world_to_texture_mat * normal;

    if (origin[3] != 0.0)
      {
      origin[0] = origin[0]/origin[3];
      origin[1] = origin[1]/origin[3];
      origin[2] = origin[2]/origin[3];
      }

    clipping_planes_ts[i]     = origin[0];
    clipping_planes_ts[i + 1] = origin[1];
    clipping_planes_ts[i + 2] = origin[2];

    clipping_planes_ts[i + 3] = normal[0];
    clipping_planes_ts[i + 4] = normal[1];
    clipping_planes_ts[i + 5] = normal[2];
    }

  /// For all samples along the ray
  while (true)
    {
    @BASE_INCREMENT@
    @TERMINATE_INCREMENT@
    @CROPPING_INCREMENT@

    for (int i = 0; i < clipping_planes_size / 6; ++i)
      {
      if (dot(vec3(l_data_pos - vec3(clipping_planes_ts[i],
                                     clipping_planes_ts[i + 1],
                                     clipping_planes_ts[i + 2])),
          vec3(clipping_planes_ts[i + 3],
               clipping_planes_ts[i + 4],
               clipping_planes_ts[i + 5])) < 0)
        {
        l_skip = true;
        break;
        }
      }

    @SHADING_INCREMENT@

    /// Advance ray by m_dir_step
    l_data_pos += l_dir_step;
    };

  @BASE_EXIT@
  @TERMINATE_EXIT@
  @CROPPING_EXIT@
  @SHADING_EXIT@
}

