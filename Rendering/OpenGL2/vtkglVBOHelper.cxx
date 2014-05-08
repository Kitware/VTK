/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCellArray.h"

#include "vtkglVBOHelper.h"

namespace vtkgl {


// Process the string, and return a version with replacements.
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



size_t CreateIndexBuffer(vtkCellArray *cells, BufferObject &indexBuffer,
                         int num)
{
  std::vector<unsigned int> indexArray;
  vtkIdType* indices(NULL);
  vtkIdType points(0);
  indexArray.reserve(cells->GetNumberOfCells() * num);
  for (cells->InitTraversal(); cells->GetNextCell(points, indices); )
    {
    if (points != num)
      {
      exit(-1); // assert(points == num);
      }
    for (int j = 0; j < num; ++j)
      {
      indexArray.push_back(static_cast<unsigned int>(*(indices++)));
      }
    }
  indexBuffer.upload(indexArray, vtkgl::BufferObject::ElementArrayBuffer);
  return indexArray.size();
}

}