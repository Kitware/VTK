#version 120


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

vec4 g_frag_color;

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms, attributes, and globals
///
//////////////////////////////////////////////////////////////////////////////
vec3 g_data_pos;
vec3 g_dir_step;

@BASE_GLOBALS_FRAG@
@TERMINATION_GLOBALS_FRAG@
@CROPPING_GLOBALS_FRAG@
@SHADING_GLOBALS_FRAG@
@BINARY_MASK_GLOBALS_FRAG@
@COMPOSITE_MASK_GLOBALS_FRAG@
@GRADIENT_OPACITY_GLOBALS_FRAG@

@COMPUTE_OPACITY_FRAG@
@COMPUTE_GRADIENT_FRAG@
@COMPUTE_LIGHTING_FRAG@
@COLOR_TRANSFER_FUNC@

@RAY_DIRECTION_FUNC_FRAG@

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
  /// Initialize g_frag_color (output) to 0
  g_frag_color = vec4(0.0);
  g_dir_step = vec3(0.0);

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
    @BINARY_MASK_INCREMENT@
    @COMPOSITE_MASK_INCREMENT@
    @SHADING_INCREMENT@

    /// Advance ray by m_dir_step
    g_data_pos += g_dir_step;
    }

  @BASE_EXIT@
  @TERMINATE_EXIT@
  @CROPPING_EXIT@
  @CLIPPING_EXIT@
  @SHADING_EXIT@

  gl_FragColor = g_frag_color;
}
