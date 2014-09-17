/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLProjectedTetrahedraMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef ___vtkVolumeShaderComposer_h
#define ___vtkVolumeShaderComposer_h

#include <vtkRenderer.h>
#include <vtkVolume.h>
#include <vtkVolumeMapper.h>
#include <vtkVolumeProperty.h>

#include <string>

namespace vtkvolume
{
  //--------------------------------------------------------------------------
  std::string replace(std::string source, const std::string &search,
                      const std::string replace, bool all)
    {
    std::string::size_type pos = 0;
    bool first = true;
    while ((pos = source.find(search, 0)) != std::string::npos)
      {
      source.replace(pos, search.length(), replace);
      pos += search.length();
      if (first)
        {
        first = false;
        if (!all)
          {
          return source;
          }
        }
      }
    return source;
    }

  //--------------------------------------------------------------------------
  std::string ComputeClip(vtkRenderer* ren, vtkVolumeMapper* mapper,
                          vtkVolume* vol)
    {
    return std::string(
      "mat4 ogl_projection_matrix = m_projection_matrix; \n\
      mat4 ogl_modelview_matrix = m_modelview_matrix; \n\
      vec4 pos = ogl_projection_matrix * ogl_modelview_matrix * m_volume_matrix * \n\
                 vec4(m_in_vertex_pos.xyz, 1); \n\
      gl_Position = pos;"
    );
    }

  //--------------------------------------------------------------------------
  std::string ComputeTextureCoords(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                   vtkVolume* vol)
    {
    return std::string(
      "/// Assuming point data only. Also, we offset the texture coordinate to account \n\
       /// for OpenGL treating voxel at the center of the cell. \n\
       vec3 uvx = (m_in_vertex_pos - m_vol_extents_min) / (m_vol_extents_max - m_vol_extents_min); \n\
       vec3 delta = m_texture_extents_max - m_texture_extents_min; \n\
       m_texture_coords = (uvx * (delta - vec3(1.0)) + vec3(0.5)) / delta;"
    );
    }

  //--------------------------------------------------------------------------
  std::string BaseGlobalsVert(vtkRenderer* ren, vtkVolumeMapper* mapper,
                               vtkVolume* vol)
    { return std::string(
    "uniform mat4 m_modelview_matrix; \n\
    uniform mat4 m_projection_matrix; \n\
    uniform mat4 m_volume_matrix; \n\
    \n\
    uniform vec3 m_vol_extents_min; \n\
    uniform vec3 m_vol_extents_max; \n\
    \n\
    uniform vec3 m_texture_extents_max; \n\
    uniform vec3 m_texture_extents_min;"
    );
    }

  //--------------------------------------------------------------------------
  std::string BaseGlobalsFrag(vtkRenderer* ren, vtkVolumeMapper* mapper,
                               vtkVolume* vol)
    {
    return std::string(
      "/// Volume dataset \n\
      uniform sampler3D m_volume; \n\
      \n\
      /// Transfer functions \n\
      uniform sampler1D m_color_transfer_func; \n\
      uniform sampler1D m_opacity_transfer_func; \n\
      \n\
      uniform sampler2D m_noise_sampler; \n\
      uniform sampler2D m_depth_sampler; \n\
      \n\
      /// Camera position \n\
      uniform vec3 m_camera_pos; \n\
      uniform vec3 m_light_pos; \n\
      \n\
      /// view and model matrices \n\
      uniform mat4 m_volume_matrix; \n\
      uniform mat4 m_inverse_volume_matrix; \n\
      uniform mat4 m_projection_matrix; \n\
      uniform mat4 m_inverse_projection_matrix; \n\
      uniform mat4 m_modelview_matrix; \n\
      uniform mat4 m_inverse_modelview_matrix; \n\
      uniform mat4 m_texture_dataset_matrix; \n\
      uniform mat4 m_inverse_texture_dataset_matrix; \n\
      \n\
      /// Ray step size \n\
      uniform vec3 m_cell_step; \n\
      uniform vec2 m_scalars_range; \n\
      uniform vec3 m_cell_spacing; \n\
      \n\
      /// Sample distance \n\
      uniform float m_sample_distance; \n\
      \n\
      /// Scales \n\
      uniform vec3 m_cell_scale; \n\
      uniform vec2 m_window_lower_left_corner; \n\
      uniform vec2 m_inv_original_window_size; \n\
      uniform vec2 m_inv_window_size; \n\
      uniform vec3 m_texture_extents_max; \n\
      uniform vec3 m_texture_extents_min; \n\
      \n\
      /// Material and lighting \n\
      uniform vec3 m_diffuse; \n\
      uniform vec3 m_ambient; \n\
      uniform vec3 m_specular; \n\
      uniform float m_shininess;"
      );
    }

  //--------------------------------------------------------------------------
  std::string BaseInit(vtkRenderer* ren, vtkVolumeMapper* mapper,
                       vtkVolume* vol)
    {
    return std::string(
      "vec4 l_light_pos_obj; \n\
      \n\
      /// Get the 3D texture coordinates for lookup into the m_volume dataset  \n\
      vec3 l_data_pos = m_texture_coords.xyz; \n\
      \n\
      /// Eye position in object space  \n\
      vec4 l_eye_pos_obj = (m_inverse_volume_matrix * vec4(m_camera_pos, 1.0)); \n\
      if (l_eye_pos_obj.w != 0.0) \n\
        { \n\
        l_eye_pos_obj.x /= l_eye_pos_obj.w; \n\
        l_eye_pos_obj.y /= l_eye_pos_obj.w; \n\
        l_eye_pos_obj.z /= l_eye_pos_obj.w; \n\
        l_eye_pos_obj.w = 1.0; \n\
        } \n\
      \n\
      /// Getting the ray marching direction (in object space); \n\
      vec3 geom_dir = normalize(m_vertex_pos.xyz - l_eye_pos_obj.xyz); \n\
      \n\
      /// Multiply the raymarching direction with the step size to get the \n\
      /// sub-step size we need to take at each raymarching step  \n\
      vec3 l_dir_step = geom_dir * m_cell_step * m_sample_distance; \n\
      \n\
      l_data_pos += l_dir_step * texture2D(m_noise_sampler, l_data_pos.xy).x;\n\
      \n\
      /// Flag to deternmine if voxel should be considered for the rendering \n\
      bool l_skip = false;");
    }

  //--------------------------------------------------------------------------
  std::string BaseIncrement(vtkRenderer* ren, vtkVolumeMapper* mapper,
                            vtkVolume* vol)
    {
    return std::string("\n\
                       l_skip = false;");
    }

  //--------------------------------------------------------------------------
  std::string BaseExit(vtkRenderer* ren, vtkVolumeMapper* mapper, vtkVolume* vol)
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string ShadingGlobalsVert(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                 vtkVolume* vol)
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string ShadingGlobalsFrag(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
                                 vtkVolume* vol)
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string ShadingInit(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
                          vtkVolume* vol)
    {
    if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
      {
      return std::string(
        "/// We get data between 0.0 - 1.0 range \n\
         float l_max_value = 0.0;");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
      {
      return std::string(
        "/// We get data between 0.0 - 1.0 range \n\
        float l_min_value = 1.0;");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
      {
      return std::string(
        "/// We get data between 0.0 - 1.0 range \n\
        float l_sum_value = 0.0;");
      }
    else if (vol->GetProperty()->GetShade())
      {
      return std::string(
        "/// Light position in object space \n\
         l_light_pos_obj = (m_inverse_volume_matrix *  vec4(m_light_pos, 1.0)); \n\
         if (l_light_pos_obj.w != 0.0) \n\
          { \n\
          l_light_pos_obj.x /= l_light_pos_obj.w; \n\
          l_light_pos_obj.y /= l_light_pos_obj.w; \n\
          l_light_pos_obj.z /= l_light_pos_obj.w; \n\
          l_light_pos_obj.w = 1.0; \n\
          };");
      }
    else
      {
      return std::string("");
      }
    }

  //--------------------------------------------------------------------------
  std::string ShadingIncrement(vtkRenderer* ren, vtkVolumeMapper* mapper,
                               vtkVolume* vol)
    {
    std::string shaderStr = "if (!l_skip) \n\
                               {\n";

    if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
      {
      shaderStr += std::string(
        "float scalar = texture3D(m_volume, l_data_pos).r; \n\
         l_max_value = max(l_max_value, scalar);");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
      {
      shaderStr += std::string(
        "float scalar = texture3D(m_volume, l_data_pos).r ; \n\
         l_min_value = min(l_min_value, scalar);");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
      {
      shaderStr += std::string(
        "float scalar = texture3D(m_volume, l_data_pos).r ; \n\
        vec4 opacity = texture1D(m_opacity_transfer_func, scalar); \n\
        l_sum_value = l_sum_value + opacity.a * scalar;");
      }
    else
      {
      shaderStr += std::string(
        "/// Data fetching from the red channel of volume texture \n\
        float scalar = texture3D(m_volume, l_data_pos).r; \n\
        vec4 l_src_color = vec4(texture1D(m_color_transfer_func, scalar).xyz, \n\
                                texture1D(m_opacity_transfer_func, scalar).w);");

      if (vol->GetProperty()->GetShade())
        {
        shaderStr += std::string(" \n\
                vec3 g1; \n\
                vec3 g2; \n\
                vec3 ldir = normalize(l_light_pos_obj.xyz - m_vertex_pos); \n\
                vec3 vdir = normalize(l_eye_pos_obj.xyz - m_vertex_pos); \n\
                vec3 h = normalize(ldir + vdir); \n\
                vec3 xvec = vec3(m_cell_step[0], 0.0, 0.0); \n\
                vec3 yvec = vec3(0.0, m_cell_step[1], 0.0); \n\
                vec3 zvec = vec3(0.0, 0.0, m_cell_step[2]); \n\
                g1.x = texture3D(m_volume, vec3(l_data_pos + xvec)).x; \n\
                g1.y = texture3D(m_volume, vec3(l_data_pos + yvec)).x; \n\
                g1.z = texture3D(m_volume, vec3(l_data_pos + zvec)).x; \n\
                g2.x = texture3D(m_volume, vec3(l_data_pos - xvec)).x; \n\
                g2.y = texture3D(m_volume, vec3(l_data_pos - yvec)).x; \n\
                g2.z = texture3D(m_volume, vec3(l_data_pos - zvec)).x; \n\
                g1.x = m_scalars_range[0] + (m_scalars_range[1] - m_scalars_range[0]) * g1.x; \n\
                g1.y = m_scalars_range[0] + (m_scalars_range[1] - m_scalars_range[0]) * g1.y; \n\
                g1.z = m_scalars_range[0] + (m_scalars_range[1] - m_scalars_range[0]) * g1.z; \n\
                g2.x = m_scalars_range[0] + (m_scalars_range[1] - m_scalars_range[0]) * g2.x; \n\
                g2.y = m_scalars_range[0] + (m_scalars_range[1] - m_scalars_range[0]) * g2.y; \n\
                g2.z = m_scalars_range[0] + (m_scalars_range[1] - m_scalars_range[0]) * g2.z; \n\
                g2 = g1 - g2; \n\
                vec3 m_spacing = vec3(m_cell_spacing[0], m_cell_spacing[1], m_cell_spacing[2]); \n\
                vec3 aspect; \n\
                float avg_spacing = (m_spacing[0] + m_spacing[1] + m_spacing[2])/3.0; \n\
                // Adjust the aspect \n\
                aspect.x = m_spacing[0] * 2.0 / avg_spacing; \n\
                aspect.y = m_spacing[1] * 2.0 / avg_spacing; \n\
                aspect.z = m_spacing[2] * 2.0 / avg_spacing; \n\
                g2.x /= aspect.x; \n\
                g2.y /= aspect.y; \n\
                g2.z /= aspect.z; \n\
                float grad_mag = sqrt(g2.x * g2.x  + g2.y * g2.y + g2.z * g2.z); \n\
                if (grad_mag > 0.0) \n\
                   { \n\
                   g2.x /= grad_mag; \n\
                   g2.y /= grad_mag; \n\
                   g2.z /= grad_mag; \n\
                   } \n\
                 else \n\
                   { \n\
                   g2 = vec3(0.0, 0.0, 0.0); \n\
                   } \n\
                grad_mag = grad_mag * 1.0 / (0.25 * (m_scalars_range[1] - (m_scalars_range[0]))); \n\
                grad_mag = clamp(grad_mag, 0.0, 1.0); \n\
                vec3 final_color = vec3(0.0); \n\
                float n_dot_l = dot(g2, ldir); \n\
                float n_dot_h = dot(g2, h); \n\
                if (n_dot_l < 0.0) \n\
                  { \n\
                  n_dot_l = -n_dot_l; \n\
                  } \n\
                if (n_dot_h < 0.0) \n\
                  { \n\
                  n_dot_h = -n_dot_h; \n\
                  } \n\
                final_color += m_ambient; \n\
                final_color += m_diffuse * n_dot_l * l_src_color.rgb; \n\
                final_color += m_specular * pow(n_dot_h, m_shininess); \n\
                final_color = clamp(final_color, l_clamp_min, l_clamp_max); \n\
                l_src_color.rgb = final_color; \n\
                @GRADIENT_OPACITY_INCREMENT@ ");
        }


      shaderStr += std::string(
      "/// Opacity calculation using compositing: \n\
      /// here we use front to back compositing scheme whereby the current sample \n\
      /// value is multiplied to the currently accumulated alpha and then this product \n\
      /// is subtracted from the sample value to get the alpha from the previous steps. \n\
      /// Next, this alpha is multiplied with the current sample colour and accumulated \n\
      /// to the composited colour. The alpha value from the previous steps is then \n\
      /// accumulated to the composited colour alpha. \n\
      l_src_color.rgb *= l_src_color.a; \n\
      m_frag_color = (1.0f - m_frag_color.a) * l_src_color + m_frag_color;"
      );
      }

    shaderStr += std::string("\n}");
    return shaderStr;
    }

  //--------------------------------------------------------------------------
  std::string GradientOpacityGlobalsFrag(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
                                         vtkVolume* vol)
    {
    if (vol->GetProperty()->GetGradientOpacity())
      {
      return std::string("uniform sampler1D m_gradient_transfer_func;");
      }

    return std::string("");
    }


  //--------------------------------------------------------------------------
  std::string GradientOpacityIncrement(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
                                       vtkVolume* vol)
    {
    if (!vol->GetProperty()->GetDisableGradientOpacity())
      {
      return std::string("l_src_color.a *= \n\
                         texture1D(m_gradient_transfer_func, grad_mag).w;");
      }

    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string ShadingExit(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
                          vtkVolume* vtkNotUsed(vol))
    {
    if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
      {
      return std::string(
       "vec4 l_src_color = texture1D(m_color_transfer_func, l_max_value); \n\
        l_src_color.a = texture1D(m_opacity_transfer_func, l_max_value).w; \n\
        m_frag_color.rgb = l_src_color.rgb * l_src_color.a; \n\
        m_frag_color.a = l_src_color.a;");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
      {
      return std::string(
        "vec4 l_src_color = texture1D(m_color_transfer_func, l_min_value); \n\
        l_src_color.a = texture1D(m_opacity_transfer_func, l_min_value).w; \n\
        m_frag_color.rgb = l_src_color.rgb * l_src_color.a; \n\
        m_frag_color.a = l_src_color.a;");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
      {
      return std::string(
        "l_sum_value = clamp(l_sum_value, 0.0, 1.0); \n\
        m_frag_color = vec4(vec3(l_sum_value), 1.0);");
      }
    else
      {
      return std::string("");
      }
  }

  //--------------------------------------------------------------------------
  std::string TerminationGlobalsVert(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                     vtkVolume* vol)
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string TerminationGlobalsFrag(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                     vtkVolume* vol)
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string TerminationInit(vtkRenderer* ren, vtkVolumeMapper* mapper,
                              vtkVolume* vol)
    {
    return std::string(
      "/// Minimum texture access coordinate \n\
      const vec3 l_tex_min = vec3(0); \n\
      \n\
      /// Maximum texture access coordinate \n\
      const vec3 l_tex_max = vec3(1); \n\
      \n\
      const vec3 l_clamp_min = vec3(0.0); \n\
      const vec3 l_clamp_max = vec3(1.0); \n\
      \n\
      /// Flag to indicate if the raymarch loop should terminate \n\
      bool stop = false; \n\
      \n\
      /// 2D Texture fragment coordinates [0,1] from fragment coordinates \n\
      /// the frame buffer texture has the size of the plain buffer but \n\
      /// we use a fraction of it. The texture coordinates is less than 1 if \n\
      /// the reduction factor is less than 1. \n\
      /// Device coordinates are between -1 and 1. We need texture coordinates \n\
      /// between 0 and 1 the m_depth_sampler buffer has the original size buffer. \n\
      vec2 m_frag_tex_coord = (gl_FragCoord.xy - m_window_lower_left_corner) * \n\
                               m_inv_window_size; \n\
      vec4 l_depth_value = texture2D(m_depth_sampler, m_frag_tex_coord); \n\
      float m_terminate_point_max = 0.0; \n\
      \n\
      /// Depth test \n\
      if(gl_FragCoord.z >= l_depth_value.x) \n\
       { \n\
       discard; \n\
       } \n\
      \n\
      /// color buffer or max scalar buffer have a reduced size. \n\
      m_frag_tex_coord = (gl_FragCoord.xy - m_window_lower_left_corner) * \n\
                           m_inv_original_window_size; \n\
      \n\
      /// Compute max number of iterations it will take before we hit \n\
      /// the termination point \n\
      \n\
      /// Abscissa of the point on the depth buffer along the ray. \n\
      /// point in texture coordinates \n\
      vec4 m_terminate_point; \n\
      m_terminate_point.x = (gl_FragCoord.x - m_window_lower_left_corner.x) * 2.0 * \n\
                            m_inv_window_size.x - 1.0; \n\
      m_terminate_point.y = (gl_FragCoord.y - m_window_lower_left_corner.y) * 2.0 * \n\
                            m_inv_window_size.y - 1.0; \n\
      m_terminate_point.z = (2.0 * l_depth_value.x - (gl_DepthRange.near + \n\
                            gl_DepthRange.far)) / gl_DepthRange.diff; \n\
      m_terminate_point.w = 1.0; \n\
      \n\
      /// From normalized device coordinates to eye coordinates. m_projection_matrix \n\
      /// is inversed because of way VT \n\
      /// From eye coordinates to texture coordinates \n\
      m_terminate_point = m_inverse_texture_dataset_matrix * \n\
                          m_inverse_volume_matrix * m_inverse_modelview_matrix * \n\
                          m_inverse_projection_matrix * \n\
                          m_terminate_point; \n\
      m_terminate_point /= m_terminate_point.w; \n\
      \n\
      m_terminate_point_max = length(m_terminate_point.xyz - l_data_pos.xyz) / \n\
                              length(l_dir_step); \n\
      float m_current_t = 0.0;");
    }

  //--------------------------------------------------------------------------
  std::string TerminationIncrement(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                   vtkVolume* vol)
    {
    return std::string(
      "/// The two constants l_tex_min and l_tex_max have a value of vec3(-1,-1,-1) \n\
      /// and vec3(1,1,1) respectively. To determine if the data value is \n\
      /// outside the m_volume data, we use the sign function. The sign function \n\
      /// return -1 if the value is less than 0, 0 if the value is equal to 0 \n\
      /// and 1 if value is greater than 0. Hence, the sign function for the \n\
      /// calculation (sign(l_data_pos-l_tex_min) and sign (l_tex_max-l_data_pos)) will \n\
      /// give us vec3(1,1,1) at the possible minimum and maximum position. \n\
      /// When we do a dot product between two vec3(1,1,1) we get the answer 3. \n\
      /// So to be within the dataset limits, the dot product will return a \n\
      /// value less than 3. If it is greater than 3, we are already out of \n\
      /// the m_volume dataset \n\
      stop = dot(sign(l_data_pos - l_tex_min), sign(l_tex_max - l_data_pos)) < 3.0; \n\
      \n\
      /// If the stopping condition is true we brek out of the ray marching loop \n\
      if (stop) \n\
       { \n\
       break; \n\
       } \n\
      /// Early ray termination \n\
      /// if the currently composited colour alpha is already fully saturated \n\
      /// we terminated the loop or if we have hit an obstacle in the direction of \n\
      /// they ray (using depth buffer) we terminate as well. \n\
      if((m_frag_color.a > (1 - 1/255.0)) ||  \n\
          m_current_t >= m_terminate_point_max) \n\
        { \n\
        break; \n\
        } \n\
      ++m_current_t;"
    );
    }

  //--------------------------------------------------------------------------
  std::string TerminationExit(vtkRenderer* ren, vtkVolumeMapper* mapper,
                              vtkVolume* vol)
   {
    return std::string("");
   }

  //--------------------------------------------------------------------------
  std::string CroppingGlobalsVert(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                  vtkVolume* vol)
  {
    return std::string("");
  }

  //--------------------------------------------------------------------------
  std::string CroppingGlobalsFrag(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                  vtkVolume* vol)
  {
    if (!mapper->GetCropping()) {
      return std::string("");
    }

    return std::string("\n\
      uniform float cropping_planes[6]; \n\
      uniform int cropping_flags [32]; \n\
      /// X: axis = 0, Y: axis = 1, Z: axis = 2 \n\
      /// cp Cropping plane bounds (minX, maxX, minY, maxY, minZ, maxZ) \n\
      int computeRegionCoord(float cp[6], vec3 pos, int axis) \n\
      { \n\
        int cpmin = axis * 2; \n\
        int cpmax = cpmin + 1; \n\
        \n\
        if (pos[axis] < cp[cpmin]) \n\
          { \n\
          return 1; \n\
          } \n\
        else if (pos[axis] >= cp[cpmin] && \n\
                 pos[axis]  < cp[cpmax]) \n\
          { \n\
          return 2; \n\
          } \n\
        else if (pos[axis] >= cp[cpmax]) \n\
          { \n\
          return 3; \n\
          } \n\
        return 0; \n\
      } \n\
      \n\
      int computeRegion(float cp[6], vec3 pos) \n\
      { \n\
        return ( computeRegionCoord(cp, pos, 0) +  \n\
                (computeRegionCoord(cp, pos, 1) - 1) * 3 + \n\
                (computeRegionCoord(cp, pos, 2) - 1) * 9); \n\
      }");
  }

  //--------------------------------------------------------------------------
  std::string CroppingInit(vtkRenderer* ren, vtkVolumeMapper* mapper,
                           vtkVolume* vol)
  {
    if (!mapper->GetCropping()) {
      return std::string("");
    }

    return std::string("\n\
      /// Convert cropping region to texture space \n\
      float cropping_planes_ts[6];\n\
      mat4  datasetToTextureMat = m_inverse_texture_dataset_matrix; \n\
      vec4 temp = vec4(cropping_planes[0], cropping_planes[1], 0.0, 1.0); \n\
      temp = datasetToTextureMat * temp; \n\
      if (temp[3] != 0.0) {temp[0] /= temp[3]; temp[1] /= temp[3];} \n\
      cropping_planes_ts[0] = temp[0];\n\
      cropping_planes_ts[1] = temp[1];\n\
      \n\
      temp = vec4(cropping_planes[2], cropping_planes[3], 0.0, 1.0); \n\
      temp = datasetToTextureMat * temp; \n\
      if (temp[3] != 0.0) {temp[0] /= temp[3]; temp[1] /= temp[3];} \n\
      cropping_planes_ts[2] = temp[0];\n\
      cropping_planes_ts[3] = temp[1];\n\
      \n\
      temp = vec4(cropping_planes[4], cropping_planes[5], 0.0, 1.0); \n\
      temp = datasetToTextureMat * temp; \n\
      if (temp[3] != 0.0) {temp[0] /= temp[3]; temp[1] /= temp[3];} \n\
      cropping_planes_ts[4] = temp[0];\n\
      cropping_planes_ts[5] = temp[1];");
  }

  //--------------------------------------------------------------------------
  std::string CroppingIncrement(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                vtkVolume* vol)
  {
    if (!mapper->GetCropping()) {
      return std::string("");
    }

    return std::string("\n\
      /// Determine region \n\
      int regionNo = computeRegion(cropping_planes_ts, l_data_pos); \n\
      \n\
      /// Do & operation with cropping flags \n\
      /// Pass the flag that its Ok to sample or not to sample \n\
      if (cropping_flags[regionNo] == 0) \n\
       { \n\
       /// Skip this voxel \n\
       l_skip = true; \n\
       }");
  }

  //--------------------------------------------------------------------------
  std::string CroppingExit(vtkRenderer* ren, vtkVolumeMapper* mapper,
                           vtkVolume* vol)
  {
    return std::string("");
  }

  //--------------------------------------------------------------------------
  std::string ClippingGlobalsVert(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                  vtkVolume* vol)
  {
    return std::string("");
  }

  //--------------------------------------------------------------------------
  std::string ClippingGlobalsFrag(vtkRenderer* ren, vtkVolumeMapper* mapper,
                                  vtkVolume* vol)
  {
    return std::string("");
  }

  //--------------------------------------------------------------------------
  std::string ClippingInit(vtkRenderer* ren, vtkVolumeMapper* mapper,
                           vtkVolume* vol)
  {
    if (!mapper->GetClippingPlanes())
      {
      return std::string("");
      }
   else
      {
      return std::string("\n\
        float clipping_planes_ts[48];\n\
        int clipping_planes_size = int(m_clipping_planes[0]);\n\
        \n\
        mat4 world_to_texture_mat = m_inverse_texture_dataset_matrix *\n\
                                    m_inverse_volume_matrix;\n\
        for (int i = 0; i < clipping_planes_size; i = i + 6)\n\
          {\n\
          vec4 origin = vec4(m_clipping_planes[i + 1],\n\
                             m_clipping_planes[i + 2],\n\
                             m_clipping_planes[i + 3], 1.0);\n\
          vec4 normal = vec4(m_clipping_planes[i + 4],\n\
                             m_clipping_planes[i + 5],\n\
                             m_clipping_planes[i + 6], 0.0);\n\
          \n\
          origin = world_to_texture_mat * origin;\n\
          normal = world_to_texture_mat * normal;\n\
          \n\
          if (origin[3] != 0.0)\n\
            {\n\
            origin[0] = origin[0] / origin[3];\n\
            origin[1] = origin[1] / origin[3];\n\
            origin[2] = origin[2] / origin[3];\n\
            }\n\
          if (normal[3] != 0.0)\n\
            {\n\
            normal[0] = normal[0] / normal[3];\n\
            normal[1] = normal[1] / normal[3];\n\
            normal[2] = normal[2] / normal[3];\n\
            }\n\
          \n\
          clipping_planes_ts[i]     = origin[0];\n\
          clipping_planes_ts[i + 1] = origin[1];\n\
          clipping_planes_ts[i + 2] = origin[2];\n\
          \n\
          clipping_planes_ts[i + 3] = normal[0];\n\
          clipping_planes_ts[i + 4] = normal[1];\n\
          clipping_planes_ts[i + 5] = normal[2];\n\
          }");
      }
  }

  //--------------------------------------------------------------------------
  std::string ClippingIncrement(vtkRenderer* vtkNotUsed(ren),
                                vtkVolumeMapper* mapper,
                                vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetClippingPlanes())
      {
      return std::string("");
      }
    else
      {
      return std::string("\n\
        for (int i = 0; i < (clipping_planes_size) && !l_skip; i = i + 6)\n\
         {\n\
         if (dot(vec3(l_data_pos - vec3(clipping_planes_ts[i],\n\
                                        clipping_planes_ts[i + 1],\n\
                                        clipping_planes_ts[i + 2])),\n\
             vec3(clipping_planes_ts[i + 3],\n\
                  clipping_planes_ts[i + 4],\n\
                  clipping_planes_ts[i + 5])) < 0)\n\
           {\n\
           l_skip = true;\n\
           break;\n\
           }\n\
         }");
      }
  }

  //--------------------------------------------------------------------------
  std::string ClippingExit(vtkRenderer* ren, vtkVolumeMapper* mapper,
                           vtkVolume* vol)
  {
    return std::string("");
  }
}


#endif // ___vtkVolumeShaderComposer_h
