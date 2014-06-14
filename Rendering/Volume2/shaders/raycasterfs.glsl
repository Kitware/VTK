#version 330 core

//////////////////////////////////////////////////////////////////////////////
///
/// Outputs
///
//////////////////////////////////////////////////////////////////////////////

/// Fragment shader output
layout(location = 0) out vec4 dst;

/// 3D texture coordinates form vertex shader
in vec3 texture_coords;

in vec3 vertex_pos;

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms
///
//////////////////////////////////////////////////////////////////////////////

/// Volume dataset
uniform sampler3D volume;

/// Transfer functions
uniform sampler1D color_transfer_func;
uniform sampler1D opacity_transfer_func;

uniform sampler2D noise;
uniform sampler2D depth;

/// Camera position
uniform vec3 camera_pos;
uniform vec3 light_pos;

/// view and model matrices
uniform mat4 scene_matrix;
uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 texture_dataset_matrix;

/// Ray step size
uniform vec3 step_size;

/// Sample distance
uniform float sample_distance;

/// Scales
uniform vec3 cell_scale;
uniform float scale;

/// Material and lighting
uniform vec3 diffuse;
uniform vec3 ambient;
uniform vec3 specular;
uniform float shininess;

uniform vec2 window_lower_left_corner;
uniform vec2 inv_original_window_size;
uniform vec2 inv_window_size;

//////////////////////////////////////////////////////////////////////////////
///
/// Constants
///
//////////////////////////////////////////////////////////////////////////////

/// Total samples for each ray march step
const int MAX_SAMPLES = 1024;

/// Minimum texture access coordinate
const vec3 tex_min = vec3(0);

/// Maximum texture access coordinate
const vec3 tex_max = vec3(1);

/// Globals
vec3 data_pos;

const vec3 clamp_min = vec3(0.0);
const vec3 clamp_max = vec3(1.0);

mat4 ogl_scene_matrix;

vec3 light_pos_obj;
vec3 eye_pos_obj;

/// 2D Texture fragment coordinates [0,1] from fragment coordinates
/// the frame buffer texture has the size of the plain buffer but
/// we use a fraction of it. The texture coordinates is less than 1 if
/// the reduction factor is less than 1.
vec2 fragTexCoord;

//////////////////////////////////////////////////////////////////////////////
///
/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
{
  /// Device coordinates are between -1 and 1. We need texture coordinates
  /// between 0 and 1 the depth buffer has the original size buffer.
  fragTexCoord = (gl_FragCoord.xy - window_lower_left_corner) * inv_window_size;
  vec4 depthValue = texture2D(depth, fragTexCoord);
  float tMax = 0.0;

  /// Depth test
  if(gl_FragCoord.z >= depthValue.x)
    {
    discard;
    }

  /// inverse is available only on 120 or above
  ogl_scene_matrix = inverse(transpose(scene_matrix));

  /// color buffer or max scalar buffer have a reduced size.
  fragTexCoord = (gl_FragCoord.xy - window_lower_left_corner) *
                 inv_original_window_size;

  /// Abscissa of the point on the depth buffer along the ray.
  /// point in texture coordinates
  vec4 maxPoint;

  maxPoint.x = (gl_FragCoord.x - window_lower_left_corner.x) * 2.0 * inv_window_size.x - 1.0;
  maxPoint.y = (gl_FragCoord.y - window_lower_left_corner.y) * 2.0 * inv_window_size.y - 1.0;
  maxPoint.z = (2.0 * depthValue.x - (gl_DepthRange.near + gl_DepthRange.far)) / gl_DepthRange.diff;
  maxPoint.w = 1.0;

  /// From normalized device coordinates to eye coordinates. projection_matrix
  /// is inversed because of way VT
  /// From eye coordinates to texture coordinates
  maxPoint = inverse(transpose(texture_dataset_matrix)) * ogl_scene_matrix * inverse(transpose(modelview_matrix)) *
             inverse(transpose(projection_matrix)) *
             maxPoint;
  maxPoint /= maxPoint.w;

  /// Get the 3D texture coordinates for lookup into the volume dataset
  data_pos = texture_coords.xyz;

  /// Eye position in object space
  eye_pos_obj = (ogl_scene_matrix * vec4(camera_pos, 1.0)).xyz;

  /// Getting the ray marching direction (in object space);
  vec3 geom_dir = normalize(vertex_pos.xyz - eye_pos_obj);

  /// Multiply the raymarching direction with the step size to get the
  /// sub-step size we need to take at each raymarching step
  vec3 dir_step = geom_dir * step_size * sample_distance;

  /// Flag to indicate if the raymarch loop should terminate
  bool stop = false;

  /// Light position in object space
  light_pos_obj = (ogl_scene_matrix *  vec4(light_pos, 1.0)).xyz;
  data_pos += dir_step * texture(noise, data_pos.xy).x;

  /// Initialize dst (output) to 0
  dst = vec4(0.0);

  tMax = length(maxPoint.xyz - data_pos.xyz) / length(dir_step);
  float t = 0.0;

  /// For all samples along the ray
  for (int i = 0; i < MAX_SAMPLES; ++i)
    {
    /// The two constants tex_min and tex_max have a value of vec3(-1,-1,-1)
    /// and vec3(1,1,1) respectively. To determine if the data value is
    /// outside the volume data, we use the sign function. The sign function
    /// return -1 if the value is less than 0, 0 if the value is equal to 0
    /// and 1 if value is greater than 0. Hence, the sign function for the
    /// calculation (sign(data_pos-tex_min) and sign (tex_max-data_pos)) will
    /// give us vec3(1,1,1) at the possible minimum and maximum position.
    /// When we do a dot product between two vec3(1,1,1) we get the answer 3.
    /// So to be within the dataset limits, the dot product will return a
    /// value less than 3. If it is greater than 3, we are already out of
    /// the volume dataset
    stop = dot(sign(data_pos - tex_min), sign(tex_max - data_pos)) < 3.0;

    //if the stopping condition is true we brek out of the ray marching loop
    if (stop)
      {
      break;
      }

    /// Data fetching from the red channel of volume texture
    float scalar = texture(volume, data_pos).r * scale;
    vec4 src = vec4(texture(color_transfer_func, scalar).xyz,
                    texture(opacity_transfer_func, scalar).w);

    /// Perform shading if enabled or else no op
    @SHADING@

    /// Opacity calculation using compositing:
    /// here we use front to back compositing scheme whereby the current sample
    /// value is multiplied to the currently accumulated alpha and then this product
    /// is subtracted from the sample value to get the alpha from the previous steps.
    /// Next, this alpha is multiplied with the current sample colour and accumulated
    /// to the composited colour. The alpha value from the previous steps is then
    /// accumulated to the composited colour alpha.
    src.rgb *= src.a;

    dst = (1.0f - dst.a) * src + dst;

    /// Early ray termination
    /// if the currently composited colour alpha is already fully saturated
    /// we terminated the loop or if we have hit an obstacle in the direction of
    /// they ray (using depth buffer) we terminate as well.
    if((dst.a > (1 - 1/255.0)) || t >= tMax)
      {
      break;
      }

    ++t;

    /// Advance ray by dir_step
    data_pos += dir_step;
    }
}
