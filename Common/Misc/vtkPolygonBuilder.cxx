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
#include "vtkIdListCollection.h"

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

void vtkPolygonBuilder::GetPolygons(vtkIdListCollection* polys)
{
  polys->RemoveAllItems();

  // We now have exactly two instances of each outer edge, corresponding to a
  // clockwise and counterclockwise traversal
  if (this->Edges.size()<6)
    {
    return;
    }

  while (!(this->Edges.empty()))
    {
    vtkIdList* poly = vtkIdList::New();

    Edge edge = *(this->Edges.begin());
    EdgeMap::iterator edgeIt = this->Edges.begin();

    vtkIdType firstVtx = edge.first;

    do
      {
      poly->InsertNextId(edge.first);

      // we will find either the inverse to our current edge, or the next edge
      // in the path.
      std::pair<EdgeMap::iterator, EdgeMap::iterator> range =
        this->Edges.equal_range(edge.second);

      edgeIt = range.first;
      vtkIdType previousVtx = edge.first;
      while (edgeIt != range.second)
        {
        if ((*edgeIt).second != previousVtx)
          {
          // we have found the next edge in the path
          edge.first = edge.second;
          edge.second = edgeIt->second;
          }
        this->Edges.erase(edgeIt++);
        }
      }
    while (edge.first != firstVtx);

    polys->AddItem(poly);
    }

  this->Reset();
}

void vtkPolygonBuilder::Reset()
{
  this->EdgeCounter.clear();
  this->Edges.clear();
}
