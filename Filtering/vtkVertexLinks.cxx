/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertexLinks.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVertexLinks.h"
#include "vtkObjectFactory.h"
#include "stdlib.h"

#include <freerange/freerange>
#include <vtkstd/vector>
using vtkstd::vector;

struct vtkVertexLinksInternals
{
  struct vtkVertex {
    vtkVertex() : InDegree(0), Degree(0), Allocated(0), Adjacent(-1) { }
    vtkIdType InDegree;
    vtkIdType Degree;
    vtkIdType Allocated;
    vtkIdType Adjacent;
  };

  vector<vtkVertex> Vertices;
  freerange<vtkIdType, vtkIdType, -1> FreeRange;
};


vtkCxxRevisionMacro(vtkVertexLinks, "1.1");
vtkStandardNewMacro(vtkVertexLinks);

//----------------------------------------------------------------------------
vtkVertexLinks::vtkVertexLinks()
{
  this->Internals = new vtkVertexLinksInternals();
}

//----------------------------------------------------------------------------
vtkVertexLinks::~vtkVertexLinks()
{
  if (Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkVertexLinks::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);  
  os << indent << "Vertices Size: " << this->Internals->Vertices.size() << endl;
}

//----------------------------------------------------------------------------
void vtkVertexLinks::Reset()
{
  this->Internals->FreeRange.clear();
  this->Internals->Vertices.clear();
}

//----------------------------------------------------------------------------
unsigned long vtkVertexLinks::GetActualMemorySize()
{
  unsigned long size = 0;
  vtkIdType vertices = this->GetNumberOfVertices();
  for (vtkIdType i = 0; i < vertices; i++)
    {
    size += this->Internals->Vertices[i].Allocated;
    }
  size *= sizeof(vtkIdType*);
  size += this->Internals->Vertices.size()*sizeof(vtkVertexLinksInternals::vtkVertex);

  return (unsigned long) ceil((float)size/1000.0); //kilobytes
}

//----------------------------------------------------------------------------
void vtkVertexLinks::DeepCopy(vtkVertexLinks* src)
{
  this->Internals->Vertices.clear();
  this->Internals->Vertices.resize(src->Internals->Vertices.size());
  this->Internals->FreeRange.clear();
  vtkIdType vertices = this->GetNumberOfVertices();
  for (vtkIdType i = 0; i < vertices; i++)
    {
    this->Internals->Vertices[i] = src->Internals->Vertices[i];
    this->Internals->Vertices[i].Adjacent = this->Internals->FreeRange.grab(this->Internals->Vertices[i].Allocated);
    for (int j = 0; j < src->Internals->Vertices[i].Degree; j++)
      {
      this->Internals->FreeRange[this->Internals->Vertices[i].Adjacent + j] = 
        src->Internals->FreeRange[src->Internals->Vertices[i].Adjacent + j];
      }
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkVertexLinks::GetDegree(vtkIdType vertex)
{
  return this->Internals->Vertices[vertex].Degree;
}

//----------------------------------------------------------------------------
void vtkVertexLinks::GetAdjacent(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges)
{
  nedges = this->Internals->Vertices[vertex].Degree;
  edges = this->Internals->FreeRange.pointer(this->Internals->Vertices[vertex].Adjacent);
}

//----------------------------------------------------------------------------
vtkIdType vtkVertexLinks::GetOutDegree(vtkIdType vertex)
{
  return this->Internals->Vertices[vertex].Degree - this->Internals->Vertices[vertex].InDegree;
}

//----------------------------------------------------------------------------
void vtkVertexLinks::GetOutAdjacent(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges)
{
  nedges = this->GetOutDegree(vertex);
  edges = this->Internals->FreeRange.pointer(this->Internals->Vertices[vertex].Adjacent + this->Internals->Vertices[vertex].InDegree);
}

//----------------------------------------------------------------------------
vtkIdType vtkVertexLinks::GetInDegree(vtkIdType vertex)
{
  return this->Internals->Vertices[vertex].InDegree;
}

//----------------------------------------------------------------------------
void vtkVertexLinks::GetInAdjacent(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges)
{
  nedges = this->GetInDegree(vertex);
  edges = this->Internals->FreeRange.pointer(this->Internals->Vertices[vertex].Adjacent);
}

//----------------------------------------------------------------------------
vtkIdType vtkVertexLinks::GetNumberOfVertices()
{
  return this->Internals->Vertices.size();
}

//----------------------------------------------------------------------------
void vtkVertexLinks::ResizeVertexList(vtkIdType vertex, vtkIdType size)
{
  vtkIdType curSize = this->Internals->Vertices[vertex].Allocated;
  //cout << "resizing Vertices[" << vertex << "] from " << curSize << " to " << size << endl;
  if (size == curSize)
    {
    return;
    }
  if (size < curSize)
    {
    if (size == 0)
      {
      this->Internals->FreeRange.free(this->Internals->Vertices[vertex].Adjacent, this->Internals->Vertices[vertex].Allocated);
      this->Internals->Vertices[vertex].Adjacent = -1;
      this->Internals->Vertices[vertex].Allocated = 0;
      }
    return;
    }
  if (size > curSize)
    {
    // If resizing, make it at least two times bigger
    if (size < 2*curSize)
      {
      size = 2*curSize;
      }

    // Resize the array
    vtkIdType arrIndex = this->Internals->FreeRange.grab(size);
    vtkIdType* arr = this->Internals->FreeRange.pointer(arrIndex);
    if (this->Internals->Vertices[vertex].Adjacent != -1)
      {
      vtkIdType* oldArr = this->Internals->FreeRange.pointer(this->Internals->Vertices[vertex].Adjacent);
      memcpy(arr, oldArr, curSize*sizeof(vtkIdType));
      this->Internals->FreeRange.free(this->Internals->Vertices[vertex].Adjacent, this->Internals->Vertices[vertex].Allocated);
      }
    this->Internals->Vertices[vertex].Adjacent = arrIndex;
    this->Internals->Vertices[vertex].Allocated = size;
    }
}


vtkIdType vtkVertexLinks::AddVertex()
{
  this->Internals->Vertices.push_back(vtkVertexLinksInternals::vtkVertex());
  return this->GetNumberOfVertices() - 1;
}

vtkIdType vtkVertexLinks::RemoveVertex(vtkIdType vertex)
{
  if (this->Internals->Vertices[vertex].Allocated > 0)
    {
    this->Internals->FreeRange.free(
      this->Internals->Vertices[vertex].Adjacent, 
      this->Internals->Vertices[vertex].Allocated);
    }
  vtkIdType movedVertex = this->GetNumberOfVertices() - 1;
  this->Internals->Vertices[vertex] = 
    this->Internals->Vertices[movedVertex];
  this->Internals->Vertices.resize(this->Internals->Vertices.size() - 1);
  return movedVertex;
}

void vtkVertexLinks::AddInAdjacent(vtkIdType vertex, vtkIdType adj)
{
  this->ResizeVertexList(vertex, this->GetDegree(vertex) + 1);
  vtkIdType adjacent = this->Internals->Vertices[vertex].Adjacent;
  this->Internals->FreeRange[adjacent + this->GetDegree(vertex)] =
    this->Internals->FreeRange[adjacent + this->GetInDegree(vertex)];
  this->Internals->FreeRange[adjacent + this->GetInDegree(vertex)] = adj;
  this->Internals->Vertices[vertex].Degree++;
  this->Internals->Vertices[vertex].InDegree++;
}

void vtkVertexLinks::AddOutAdjacent(vtkIdType vertex, vtkIdType adj)
{
  this->ResizeVertexList(vertex, this->GetDegree(vertex) + 1);
  vtkIdType adjacent = this->Internals->Vertices[vertex].Adjacent;
  this->Internals->FreeRange[adjacent + this->GetDegree(vertex)] = adj;
  this->Internals->Vertices[vertex].Degree++;
}

void vtkVertexLinks::RemoveInAdjacent(vtkIdType vertex, vtkIdType adj)
{
  vtkIdType adjacent = this->Internals->Vertices[vertex].Adjacent;
  for (vtkIdType e = 0; e < this->GetInDegree(vertex); e++)
    {
    if (this->Internals->FreeRange[adjacent + e] == adj)
      {
      this->Internals->FreeRange[adjacent + e] = 
        this->Internals->FreeRange[adjacent + this->GetInDegree(vertex) - 1];
      this->Internals->FreeRange[adjacent + this->GetInDegree(vertex) - 1] = 
        this->Internals->FreeRange[adjacent + this->GetDegree(vertex) - 1];
      this->Internals->Vertices[vertex].Degree--;
      this->Internals->Vertices[vertex].InDegree--;
      break;
      }
    }
}

void vtkVertexLinks::RemoveOutAdjacent(vtkIdType vertex, vtkIdType adj)
{
  vtkIdType adjacent = this->Internals->Vertices[vertex].Adjacent;
  for (vtkIdType e = this->GetInDegree(vertex); e < this->GetDegree(vertex); e++)
    {
    if (this->Internals->FreeRange[adjacent + e] == adj)
      {
      this->Internals->FreeRange[adjacent + e] = 
        this->Internals->FreeRange[adjacent + this->GetDegree(vertex) - 1];
      this->Internals->Vertices[vertex].Degree--;
      break;
      }
    }
}

void vtkVertexLinks::RemoveOutAdjacentShift(vtkIdType vertex, vtkIdType adj)
{
  vtkIdType adjacent = this->Internals->Vertices[vertex].Adjacent;
  for (vtkIdType e = this->GetInDegree(vertex); e < this->GetDegree(vertex); e++)
    {
    if (this->Internals->FreeRange[adjacent + e] == adj)
      {
      if (e < this->GetDegree(vertex) - 1)
        {
        vtkIdType* fromPtr = this->Internals->FreeRange.pointer(adjacent + e + 1);
        vtkIdType* toPtr = this->Internals->FreeRange.pointer(adjacent + e);
        int size = this->GetDegree(vertex) - e - 1;
        memmove(toPtr, fromPtr, size*sizeof(vtkIdType));
        }
      this->Internals->Vertices[vertex].Degree--;
      break;
      }
    }
}

vtkIdType vtkVertexLinks::GetOutAdjacent(vtkIdType vertex, vtkIdType index)
{
  return this->Internals->FreeRange[this->Internals->Vertices[vertex].Adjacent + this->Internals->Vertices[vertex].InDegree + index];
}

vtkIdType vtkVertexLinks::GetInAdjacent(vtkIdType vertex, vtkIdType index)
{
  return this->Internals->FreeRange[this->Internals->Vertices[vertex].Adjacent + index];
}

void vtkVertexLinks::SetOutAdjacent(vtkIdType vertex, vtkIdType index, vtkIdType value)
{
  this->Internals->FreeRange[this->Internals->Vertices[vertex].Adjacent + this->Internals->Vertices[vertex].InDegree + index] =
    value;
}

void vtkVertexLinks::SetInAdjacent(vtkIdType vertex, vtkIdType index, vtkIdType value)
{
  this->Internals->FreeRange[this->Internals->Vertices[vertex].Adjacent + index] =
    value;
}

