/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygonBuilder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPolygonBuilder.h"
#include "vtkIdList.h"

vtkPolygonBuilder::vtkPolygonBuilder()
{
}

void vtkPolygonBuilder::InsertTriangle(vtkIdType* abc)
{
  // For each triangle edge, and for each direction (clockwise and
  // counterclockwise): the number of instances of each edge are recorded, and
  // edges with exactly one instance are stored

  for (int i=0;i<3;i++)
    {
    for (int j=1;j>=-1;j-=2)
      {
      Edge edge(abc[i],abc[(i+j+3)%3]);
      size_t nInstances = ++(this->EdgeCounter[edge]);

      if (nInstances == 1)
        {
        this->Edges.insert(std::make_pair(edge.first,edge.second));
        }
        else if (nInstances == 2)
        {
        std::pair<EdgeMap::iterator, EdgeMap::iterator> range = Edges.equal_range(edge.first);

        EdgeMap::iterator it = range.first;
        for (; it != range.second; ++it)
          {
          if (it->second == edge.second)
            {
            Edges.erase(it);
            break;
            }
          }
        }
      }
    }
  return;
}

void vtkPolygonBuilder::GetPolygon(vtkIdList* poly)
{
  poly->Reset();

  // We now have exactly two instances of each outer edge, corresponding to a
  // clockwise and counterclockwise traversal
  if (this->Edges.size()<6)
    {
    return;
    }

  EdgeMap::iterator edgeIt = this->Edges.begin();

  vtkIdType firstVtx,vtx1,vtx2;
  firstVtx = vtx1 = (*edgeIt).first;
  poly->InsertNextId(vtx1);

  vtx2 = (*edgeIt).second;

  while (vtx2 != firstVtx)
    {
    poly->InsertNextId(vtx2);

    // we will find either the inverse to our current edge, or the next edge
    // in the path.
    edgeIt = this->Edges.find(vtx2);
    if ((*edgeIt).second == vtx1)
      {
      // we have found the inverse. Remove it and look agian.
      this->Edges.erase(edgeIt);
      edgeIt = this->Edges.find(vtx2);
      }

    vtx1 = vtx2;
    vtx2 = edgeIt->second;
    }

  this->Reset();
}

void vtkPolygonBuilder::Reset()
{
  this->EdgeCounter.clear();
  this->Edges.clear();
}
