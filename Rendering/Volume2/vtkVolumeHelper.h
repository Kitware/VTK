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


  std::string ShadeLoop()
    {
    return std::string
    ("if (m_src_color.a > 0.01) \
        { \
        vec3 g1; \
        vec3 g2; \
        vec3 ldir = normalize(light_pos_obj - vertex_pos); \
        vec3 vdir = normalize(eye_pos_obj - vertex_pos); \
        vec3 h = normalize(ldir + vdir); \
        vec3 xvec = vec3(step_size[0], 0.0, 0.0); \
        vec3 yvec = vec3(0.0, step_size[1], 0.0); \
        vec3 zvec = vec3(0.0, 0.0, step_size[2]); \
        g1.x = texture(volume, vec3(data_pos + xvec)).x; \
        g1.y = texture(volume, vec3(data_pos + yvec)).x; \
        g1.z = texture(volume, vec3(data_pos + zvec)).x; \
        g2.x = texture(volume, vec3(data_pos - xvec)).x; \
        g2.y = texture(volume, vec3(data_pos - yvec)).x; \
        g2.z = texture(volume, vec3(data_pos - zvec)).x; \
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
        final_color = clamp(final_color, clamp_min, clamp_max); \
        m_src_color.rgb += final_color.rgb; \
       }"
    );
    }

  std::string TerminateInit()
    {
    return std::string(" \
    \/\/\/ Compute max number of iterations it will take before we hit \
    \/\/\/ the termination point \
    \
    \/\/\/ Abscissa of the point on the depth buffer along the ray. \
    \/\/\/ point in texture coordinates \
    \vec4 m_terminate_point; \
    m_terminate_point.x = (gl_FragCoord.x - window_lower_left_corner.x) * 2.0 * \
                          inv_window_size.x - 1.0; \
    m_terminate_point.y = (gl_FragCoord.y - window_lower_left_corner.y) * 2.0 * \
                          inv_window_size.y - 1.0; \
    m_terminate_point.z = (2.0 * depthValue.x - (gl_DepthRange.near + \
                          gl_DepthRange.far)) / gl_DepthRange.diff; \
    m_terminate_point.w = 1.0; \
    \
    \/\/\/ From normalized device coordinates to eye coordinates. projection_matrix \
    \/\/\/ is inversed because of way VT \
    \/\/\/ From eye coordinates to texture coordinates \
    m_terminate_point = inverse(transpose(texture_dataset_matrix)) * \
                        ogl_scene_matrix * inverse(transpose(modelview_matrix)) * \
                        inverse(transpose(projection_matrix)) * \
                        m_terminate_point; \
    m_terminate_point /= m_terminate_point.w; \
    \
    m_terminate_point_max = length(m_terminate_point.xyz - data_pos.xyz) / \
                            length(dir_step); \
    float m_current_t = 0.0;"
    );
    }

  std::string TerminateLoop()
    {
    return std::string(" \
      \/\/\/ Early ray termination \
      \/\/\/ if the currently composited colour alpha is already fully saturated \
      \/\/\/ we terminated the loop or if we have hit an obstacle in the direction of \
      \/\/\/ they ray (using depth buffer) we terminate as well. \
      if((m_frag_color.a > (1 - 1/255.0)) ||  \
          m_current_t >= m_terminate_point_max) \
        { \
        break; \
        } \
      ++m_current_t;"
    );
    }

  std::string TerminateExit()
   {
    return std::string("");
   }
}


#endif // __vtkVolumeHelper_h
