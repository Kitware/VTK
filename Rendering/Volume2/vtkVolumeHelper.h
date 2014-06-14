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
}


#endif // __vtkVolumeHelper_h
