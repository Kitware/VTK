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

void vtkPolygonBuilder::Reset()
{
  this->Poly.clear();
}

bool vtkPolygonBuilder::InsertTriangle(vtkIdType* abc)
{
  if(this->Poly.size()==0)
    {
    this->Insert(0, abc[0]);
    this->Insert(0, abc[1]);
    this->Insert(1, abc[2]);
    return true;
    }

  int i;
  VertexRef v(0);
  for(i=0; i<3; i++)
    {
    if(FindEdge(abc[(i+1)%3],abc[(i+2)%3],v))
      {
      break;
      }
    }

  if(i==3)
    {
    return false;
    }

  this->Insert(v, abc[i]);
  return true;
}

void vtkPolygonBuilder::GetPolygon(vtkIdList* poly) const
{
  poly->Reset();
  if(this->Poly.size()==0)
    {
    return;
    }
  VertexRef v = 0;
  do
    {
    poly->InsertNextId(this->GetVertexId(v));
    v = this->GetNextVertex(v);
    }while(v!=0);
}

bool vtkPolygonBuilder::FindEdge(vtkIdType a, vtkIdType b, VertexRef& v) const
{
  v = 0;
  do
    {
    VertexRef u = this->GetNextVertex(v);
    if( this->GetVertexId(u)==a && this->GetVertexId(v)==b )
      {
      return true;
      }
    v = u;
    }while(v!=0);
  return false;
}

vtkPolygonBuilder::VertexRef vtkPolygonBuilder::Insert(VertexRef i, vtkIdType vertexId)
{
  //Allocate a vertex that points to itself
  VertexRef j = this->Poly.size();
  this->Poly.push_back(Vertex(j,vertexId));

  //splice the cycle
  this->Poly[j].Next = this->Poly[i].Next;
  this->Poly[i].Next = j;
  return j;
}
