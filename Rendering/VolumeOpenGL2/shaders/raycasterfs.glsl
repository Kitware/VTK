#version 120

/// Needed to enable inverse function
#extension GL_ARB_gpu_shader5 : enable

//////////////////////////////////////////////////////////////////////////////
///
/// Inputs
///
//////////////////////////////////////////////////////////////////////////////

/// 3D texture coordinates form vertex shader
varying vec3 m_texture_coords;
varying vec3 m_vertex_pos;

//////////////////////////////////////////////////////////////////////////////
///
/// Outputs
///
//////////////////////////////////////////////////////////////////////////////

/// Fragment shader output
vec4 m_frag_color;

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms, attributes, and globals
///
//////////////////////////////////////////////////////////////////////////////
@BASE_GLOBALS_FRAG@
@TERMINATION_GLOBALS_FRAG@
@CROPPING_GLOBALS_FRAG@
@SHADING_GLOBALS_FRAG@
@GRADIENT_OPACITY_GLOBALS_FRAG@

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
  @CLIPPING_INIT@

  /// For all samples along the ray
  while (true)
    {
    @BASE_INCREMENT@
    @TERMINATE_INCREMENT@
    @CROPPING_INCREMENT@
    @CLIPPING_INCREMENT@
    @SHADING_INCREMENT@

    /// Advance ray by m_dir_step
    l_data_pos += l_dir_step;
    }

  @BASE_EXIT@
  @TERMINATE_EXIT@
  @CROPPING_EXIT@
  @CLIPPING_EXIT@
  @SHADING_EXIT@

  gl_FragColor = m_frag_color;
}

