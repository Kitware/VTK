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
/// Attributes
///
//////////////////////////////////////////////////////////////////////////////

@BASE_ATTRIBUTES_FRAG@
@TERMINATION_ATTRIBUTES_FRAG@
@SHADING_ATTRIBUTES_FRAG@

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms
///
//////////////////////////////////////////////////////////////////////////////

@BASE_UNIFORMS_FRAG@
@TERMINATION_UNIFORMS_FRAG@
@SHADING_UNIFORMS_FRAG@

//////////////////////////////////////////////////////////////////////////////
///
/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
{
  vec3 m_light_pos_obj;

  /// inverse is available only on 120 or above
  mat4 m_ogl_scene_matrix = inverse(transpose(m_scene_matrix));

  /// Get the 3D texture coordinates for lookup into the m_volume dataset
  vec3 m_data_pos = m_texture_coords.xyz;

  /// Eye position in object space
  vec3 m_eye_pos_obj = (m_ogl_scene_matrix * vec4(m_camera_pos, 1.0)).xyz;

  /// Getting the ray marching direction (in object space);
  vec3 geom_dir = normalize(m_vertex_pos.xyz - m_eye_pos_obj);

  /// Multiply the raymarching direction with the step size to get the
  /// sub-step size we need to take at each raymarching step
  vec3 m_dir_step = geom_dir * m_step_size * m_sample_distance;

  m_data_pos += m_dir_step * texture(m_noise_sampler, m_data_pos.xy).x;

  /// Initialize m_frag_color (output) to 0
  m_frag_color = vec4(0.0);

  @TERMINATE_INIT@
  @SHADING_INIT@

  /// For all samples along the ray
  for (int i = 0; i < MAX_SAMPLES; ++i)
    {
    @TERMINATE_LOOP@

    /// Data fetching from the red channel of m_volume texture
    float scalar = texture(m_volume, m_data_pos).r * m_scale;
    vec4 m_src_color = vec4(texture(m_color_transfer_func, scalar).xyz,
                    texture(m_opacity_transfer_func, scalar).w);

    /// Perform shading if enabled or else no op
    @SHADING_LOOP@

    /// Opacity calculation using compositing:
    /// here we use front to back compositing scheme whereby the current sample
    /// value is multiplied to the currently accumulated alpha and then this product
    /// is subtracted from the sample value to get the alpha from the previous steps.
    /// Next, this alpha is multiplied with the current sample colour and accumulated
    /// to the composited colour. The alpha value from the previous steps is then
    /// accumulated to the composited colour alpha.
    m_src_color.rgb *= m_src_color.a;
    m_frag_color = (1.0f - m_frag_color.a) * m_src_color + m_frag_color;

    /// Advance ray by m_dir_step
    m_data_pos += m_dir_step;
    }

  @SHADING_EXIT@
  @TERMINATE_EXIT@
}
