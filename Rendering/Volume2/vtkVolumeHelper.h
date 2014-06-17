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

#ifndef __vtkVolumeHelper_h
#define __vtkVolumeHelper_h

#include <string>

namespace vtkvolume
{
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

  std::string ShadeUniforms()
    {
    return std::string(" \n\
      \/\/\/ Material and lighting \n\
      uniform vec3 m_diffuse; \n\
      uniform vec3 m_ambient; \n\
      uniform vec3 m_specular; \n\
      uniform float m_shininess;"
    );
    }

  std::string ShadeAttributes()
    {
    return std::string("");
    }

  std::string ShadeInit()
    {
    return std::string("");
    }


  std::string ShadeLoop()
    {
    return std::string
    ("if (m_src_color.a > 0.01) \
        { \
        vec3 g1; \
        vec3 g2; \
        vec3 ldir = normalize(m_light_pos_obj - m_vertex_pos); \
        vec3 vdir = normalize(m_eye_pos_obj - m_vertex_pos); \
        vec3 h = normalize(ldir + vdir); \
        vec3 xvec = vec3(m_step_size[0], 0.0, 0.0); \
        vec3 yvec = vec3(0.0, m_step_size[1], 0.0); \
        vec3 zvec = vec3(0.0, 0.0, m_step_size[2]); \
        g1.x = texture(m_volume, vec3(m_data_pos + xvec)).x; \
        g1.y = texture(m_volume, vec3(m_data_pos + yvec)).x; \
        g1.z = texture(m_volume, vec3(m_data_pos + zvec)).x; \
        g2.x = texture(m_volume, vec3(m_data_pos - xvec)).x; \
        g2.y = texture(m_volume, vec3(m_data_pos - yvec)).x; \
        g2.z = texture(m_volume, vec3(m_data_pos - zvec)).x; \
        g2 = normalize(g1 - g2); \
        float normalLength = length(g2); \
        if (normalLength > 0.0) \
          { \
          g2 = normalize(g2); \
          } \
        else \
          { \
          g2 = vec3(0.0, 0.0, 0.0); \
          } \
        vec3 final_color = vec3(0.0); \
        float n_dot_l = dot(g2, ldir); \
        float n_dot_h = dot(g2, h); \
        if (n_dot_l < 0.0) \
          { \
          n_dot_l =- n_dot_l; \
          } \
        if (n_dot_h < 0.0) \
          { \
          n_dot_h =- n_dot_h; \
          } \
        final_color += ambient; \
        final_color += diffuse * n_dot_l; \
        float shine_factor = pow(n_dot_h, shininess); \
        final_color += specular * shine_factor; \
        final_color = clamp(final_color, m_clamp_min, m_clamp_max); \
        m_src_color.rgb += final_color.rgb; \
       }"
    );
    }

  std::string ShadeExit()
    {
    return std::string("");
    }

  std::string TerminateInit()
    {
    return std::string(" \n\
    \/\/\/ Compute max number of iterations it will take before we hit \n\
    \/\/\/ the termination point \n\
    \n\
    \/\/\/ Abscissa of the point on the depth buffer along the ray. \n\
    \/\/\/ point in texture coordinates \n\
    vec4 m_terminate_point; \n\
    m_terminate_point.x = (gl_FragCoord.x - m_window_lower_left_corner.x) * 2.0 * \n\
                          m_inv_window_size.x - 1.0; \n\
    m_terminate_point.y = (gl_FragCoord.y - m_window_lower_left_corner.y) * 2.0 * \n\
                          m_inv_window_size.y - 1.0; \n\
    m_terminate_point.z = (2.0 * m_depth_value.x - (gl_DepthRange.near + \n\
                          gl_DepthRange.far)) / gl_DepthRange.diff; \n\
    m_terminate_point.w = 1.0; \n\
    \n\
    \/\/\/ From normalized device coordinates to eye coordinates. m_projection_matrix \n\
    \/\/\/ is inversed because of way VT \n\
    \/\/\/ From eye coordinates to texture coordinates \n\
    m_terminate_point = inverse(transpose(m_texture_dataset_matrix)) * \n\
                        m_ogl_scene_matrix * inverse(transpose(m_modelview_matrix)) * \n\
                        inverse(transpose(m_projection_matrix)) * \n\
                        m_terminate_point; \n\
    m_terminate_point /= m_terminate_point.w; \n\
    \n\
    m_terminate_point_max = length(m_terminate_point.xyz - m_data_pos.xyz) / \n\
                            length(m_dir_step); \n\
    float m_current_t = 0.0;"
    );
    }

  std::string TerminateLoop()
    {
    return std::string(" \n\
      \/\/\/ Early ray termination \n\
      \/\/\/ if the currently composited colour alpha is already fully saturated \n\
      \/\/\/ we terminated the loop or if we have hit an obstacle in the direction of \n\
      \/\/\/ they ray (using depth buffer) we terminate as well. \n\
      if((m_frag_color.a > (1 - 1/255.0)) ||  \n\
          m_current_t >= m_terminate_point_max) \n\
        { \n\
        break; \n\
        } \n\
      ++m_current_t;"
    );
    }

  std::string TerminateExit()
   {
    return std::string("");
   }
}


#endif // __vtkVolumeHelper_h
