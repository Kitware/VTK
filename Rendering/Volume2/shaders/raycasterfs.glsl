#version 330 core

//////////////////////////////////////////////////////////////////////////////
///
/// Outputs
///
//////////////////////////////////////////////////////////////////////////////

/// Fragment shader output
layout(location = 0) out vec4 m_frag_color;

/// 3D texture coordinates form vertex shader
in vec3 m_texture_coords;

in vec3 m_vertex_pos;

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms
///
//////////////////////////////////////////////////////////////////////////////

/// Volume dataset
uniform sampler3D m_volume;

/// Transfer functions
uniform sampler1D m_color_transfer_func;
uniform sampler1D m_opacity_transfer_func;

uniform sampler2D m_noise_sampler;
uniform sampler2D m_depth_sampler;

/// Camera position
uniform vec3 m_camera_pos;
uniform vec3 m_light_pos;

/// view and model matrices
uniform mat4 m_scene_matrix;
uniform mat4 m_projection_matrix;
uniform mat4 m_modelview_matrix;
uniform mat4 m_texture_dataset_matrix;

/// Ray step size
uniform vec3 m_step_size;

/// Sample distance
uniform float m_sample_distance;

/// Scales
uniform vec3 m_cell_m_scale;
uniform float m_scale;

@SHADING_ATTRIBUTES@
@SHADING_UNIFORMS@

uniform vec2 m_window_lower_left_corner;
uniform vec2 m_inv_original_window_size;
uniform vec2 m_inv_window_size;

//////////////////////////////////////////////////////////////////////////////
///
/// Constants
///
//////////////////////////////////////////////////////////////////////////////

/// Total samples for each ray march step
const int MAX_SAMPLES = 1024;

/// Minimum texture access coordinate
const vec3 m_tex_min = vec3(0);

/// Maximum texture access coordinate
const vec3 m_tex_max = vec3(1);

/// Globals
vec3 m_data_pos;

const vec3 m_clamp_min = vec3(0.0);
const vec3 m_clamp_max = vec3(1.0);

mat4 m_ogl_scene_matrix;

vec3 m_light_pos_obj;
vec3 m_eye_pos_obj;

/// 2D Texture fragment coordinates [0,1] from fragment coordinates
/// the frame buffer texture has the size of the plain buffer but
/// we use a fraction of it. The texture coordinates is less than 1 if
/// the reduction factor is less than 1.
vec2 m_frag_tex_coord;

//////////////////////////////////////////////////////////////////////////////
///
/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
{
  /// Device coordinates are between -1 and 1. We need texture coordinates
  /// between 0 and 1 the m_depth_sampler buffer has the original size buffer.
  m_frag_tex_coord = (gl_FragCoord.xy - m_window_lower_left_corner) *
                      m_inv_window_size;
  vec4 m_depth_value = texture2D(m_depth_sampler, m_frag_tex_coord);
  float m_terminate_point_max = 0.0;

  /// Depth test
  if(gl_FragCoord.z >= m_depth_value.x)
    {
    discard;
    }

  /// inverse is available only on 120 or above
  m_ogl_scene_matrix = inverse(transpose(m_scene_matrix));

  /// color buffer or max scalar buffer have a reduced size.
  m_frag_tex_coord = (gl_FragCoord.xy - m_window_lower_left_corner) *
                 m_inv_original_window_size;

  /// Get the 3D texture coordinates for lookup into the m_volume dataset
  m_data_pos = m_texture_coords.xyz;

  /// Eye position in object space
  m_eye_pos_obj = (m_ogl_scene_matrix * vec4(m_camera_pos, 1.0)).xyz;

  /// Getting the ray marching direction (in object space);
  vec3 geom_dir = normalize(m_vertex_pos.xyz - m_eye_pos_obj);

  /// Multiply the raymarching direction with the step size to get the
  /// sub-step size we need to take at each raymarching step
  vec3 m_dir_step = geom_dir * m_step_size * m_sample_distance;

  /// Flag to indicate if the raymarch loop should terminate
  bool stop = false;

  /// Light position in object space
  m_light_pos_obj = (m_ogl_scene_matrix *  vec4(m_light_pos, 1.0)).xyz;
  m_data_pos += m_dir_step * texture(m_noise_sampler, m_data_pos.xy).x;

  /// Initialize m_frag_color (output) to 0
  m_frag_color = vec4(0.0);

  @TERMINATE_INIT@
  @SHADING_INIT@

  /// For all samples along the ray
  for (int i = 0; i < MAX_SAMPLES; ++i)
    {
    /// The two constants m_tex_min and m_tex_max have a value of vec3(-1,-1,-1)
    /// and vec3(1,1,1) respectively. To determine if the data value is
    /// outside the m_volume data, we use the sign function. The sign function
    /// return -1 if the value is less than 0, 0 if the value is equal to 0
    /// and 1 if value is greater than 0. Hence, the sign function for the
    /// calculation (sign(m_data_pos-m_tex_min) and sign (m_tex_max-m_data_pos)) will
    /// give us vec3(1,1,1) at the possible minimum and maximum position.
    /// When we do a dot product between two vec3(1,1,1) we get the answer 3.
    /// So to be within the dataset limits, the dot product will return a
    /// value less than 3. If it is greater than 3, we are already out of
    /// the m_volume dataset
    stop = dot(sign(m_data_pos - m_tex_min), sign(m_tex_max - m_data_pos)) < 3.0;

    //if the stopping condition is true we brek out of the ray marching loop
    if (stop)
      {
      break;
      }

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


    @TERMINATE_LOOP@

    /// Advance ray by m_dir_step
    m_data_pos += m_dir_step;
    }

  @SHADING_EXIT@
}
