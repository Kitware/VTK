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
  // For each triangle edge: the number of instances of each edge are recorded,
  // and edges with exactly one instance are stored. Triangle edges are only
  // traversed in a counterclockwise direction.

  for (int i=0;i<3;i++)
    {
    Edge edge(abc[i],abc[(i+1)%3]);
    Edge inverseEdge(abc[(i+1)%3],abc[i]);

    ++(this->EdgeCounter[edge]);

    if (this->EdgeCounter[inverseEdge] == 0)
      {
      this->Edges.insert(std::make_pair(edge.first,edge.second));
      }
    else if (this->EdgeCounter[edge] == 1)
      {
      std::pair<EdgeMap::iterator,
        EdgeMap::iterator> range = Edges.equal_range(inverseEdge.first);

      EdgeMap::iterator it = range.first;
      for (; it != range.second; ++it)
        {
        if (it->second == inverseEdge.second)
          {
          Edges.erase(it);
          break;
          }
        }
      }
    }
  return;
}

void vtkPolygonBuilder::GetPolygons(vtkIdListCollection* polys)
{
  polys->RemoveAllItems();

  // We now have exactly one instance of each outer edge, corresponding to a
  // counterclockwise traversal of the polygon
  if (this->Edges.size()<3)
    {
    return;
    }

  while (!(this->Edges.empty()))
    {
    vtkIdList* poly = vtkIdList::New();

    EdgeMap::iterator edgeIt = this->Edges.begin();
    Edge edge = *(edgeIt);

    vtkIdType firstVtx = edge.first;

    do
     {
      poly->InsertNextId(edge.first);
      edgeIt = this->Edges.find(edge.second);
      edge = *(edgeIt);
      Edges.erase(edgeIt);
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
