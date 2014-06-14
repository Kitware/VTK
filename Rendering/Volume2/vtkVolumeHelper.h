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


  std::string shade()
    {
    return std::string
    ("if (src.a > 0.01) \
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
        vec3 finalColor = vec3(0.0); \
        float nDotL = dot(g2, ldir); \
        float nDotH = dot(g2, h); \
        if (nDotL < 0.0) \
          { \
          nDotL =- nDotL; \
          } \
        if (nDotH < 0.0) \
          { \
          nDotH =- nDotH; \
          } \
        finalColor += ambient; \
        finalColor += diffuse * nDotL; \
        float shininessFactor = pow(nDotH, shininess); \
        finalColor += specular * shininessFactor; \
        finalColor = clamp(finalColor, clamp_min, clamp_max); \
        src.rgb += finalColor.rgb; \
       }"
    );
    }
}


#endif // __vtkVolumeHelper_h
