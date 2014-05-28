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

/// Camera position
uniform vec3 camera_pos;
uniform vec3 light_pos;

/// view and model matrices
uniform mat4 scene_matrix;

/// Ray step size
uniform vec3 step_size;

/// Sample distance
uniform float sample_distance;

/// Scales
uniform vec3 cell_scale;
uniform float scale;

/// Enable / disable shading
uniform bool enable_shading;

/// Material and lighting
uniform vec3 diffuse;
uniform vec3 ambient;
uniform vec3 specular;
uniform float shininess;

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

//////////////////////////////////////////////////////////////////////////////
///
/// Perform shading on the volume
///
///
//////////////////////////////////////////////////////////////////////////////
vec3 shade()
{
  /// g1 - g2 is gradient in object space
  vec3 g1;
  vec3 g2;
  vec3 ldir = normalize(light_pos_obj - vertex_pos);
  vec3 vdir = normalize(eye_pos_obj - vertex_pos);
  vec3 h = normalize(ldir + vdir);

  vec3 xvec = vec3(step_size[0], 0.0, 0.0);
  vec3 yvec = vec3(0.0, step_size[1], 0.0);
  vec3 zvec = vec3(0.0, 0.0, step_size[2]);

  g1.x = texture(volume, vec3(data_pos + xvec)).x;
  g1.y = texture(volume, vec3(data_pos + yvec)).x;
  g1.z = texture(volume, vec3(data_pos + zvec)).x;

  g2.x = texture(volume, vec3(data_pos - xvec)).x;
  g2.y = texture(volume, vec3(data_pos - yvec)).x;
  g2.z = texture(volume, vec3(data_pos - zvec)).x;

  g2 = normalize(g1 - g2);

  float normalLength = length(g2);
  if(normalLength > 0.0) {
    g2 = normalize(g2);
  } else {
    g2 = vec3(0.0, 0.0, 0.0);
  }

  /// Initialize color to 1.0
  vec3 finalColor = vec3(0.0);

  /// Perform simple light calculations
  float nDotL = dot(g2, ldir);
  float nDotH = dot(g2, h);

  /// Separate nDotL and nDotH for two-sided shading, otherwise we
  /// get black spots.

  /// Two-sided shading
  if (nDotL < 0.0) {
    nDotL =- nDotL;
  }

  /// Two-sided shading
    if (nDotH < 0.0) {
      nDotH =- nDotH;
    }

  /// Ambient term for this light
  finalColor += ambient;

  /// Diffuse term for this light
  finalColor += diffuse * nDotL;

  /// Specular term for this light
  float shininessFactor = pow(nDotH, shininess);
  finalColor += specular * shininessFactor;

  /// clamp values otherwise we get black spots
  finalColor = clamp(finalColor, clamp_min, clamp_max);

  return finalColor;
}

//////////////////////////////////////////////////////////////////////////////
///
/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
{
  /// Get the 3D texture coordinates for lookup into the volume dataset
  data_pos = texture_coords.xyz;

  /// inverse is available only on 120 or above
  ogl_scene_matrix = inverse(transpose(scene_matrix));

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

  /// For all samples along the ray
  for (int i = 0; i < MAX_SAMPLES; ++i) {
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
    if (stop) {
      break;
    }

    /// Data fetching from the red channel of volume texture
    float scalar = texture(volume, data_pos).r * scale;
    vec4 src = vec4(texture(color_transfer_func, scalar).xyz,
                    texture(opacity_transfer_func, scalar).w);

    /// Opacity calculation using compositing:
    /// here we use front to back compositing scheme whereby the current sample
    /// value is multiplied to the currently accumulated alpha and then this product
    /// is subtracted from the sample value to get the alpha from the previous steps.
    /// Next, this alpha is multiplied with the current sample colour and accumulated
    /// to the composited colour. The alpha value from the previous steps is then
    /// accumulated to the composited colour alpha.
    if (src.a > 0.01 && enable_shading) {
      src.rgb += shade().rgb;
    }

    src.rgb *= src.a;
    dst = (1.0f - dst.a) * src + dst;

    /// Early ray termination
    /// if the currently composited colour alpha is already fully saturated
    /// we terminated the loop
    if(dst.a > (1 - 1/255.0)) {
      break;
    }

    /// Advance ray by dir_step
    data_pos += dir_step;
  }
}
