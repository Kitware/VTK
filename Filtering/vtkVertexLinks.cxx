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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkVertexLinks.h"
#include "vtkObjectFactory.h"
#include "stdlib.h"

#include <freerange/freerange>
#include <vtkstd/vector>
using vtkstd::vector;

struct vtkVertexLinksInternals
{
  struct vtkVertexLinkInfo {
    vtkVertexLinkInfo() : InDegree(0), Degree(0), Allocated(0), Index(-1) { }
    vtkIdType InDegree;
    vtkIdType Degree;
    vtkIdType Allocated;
    vtkIdType Index;
  };

  vector<vtkVertexLinkInfo> VertexLinkInfo;
  freerange<vtkIdType, vtkIdType, -1> AdjacencyHeap;
};


vtkCxxRevisionMacro(vtkVertexLinks, "1.5");
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
  os << indent << "VertexLinkInfo Size: " << this->Internals->VertexLinkInfo.size() << endl;
}

//----------------------------------------------------------------------------
void vtkVertexLinks::Reset()
{
  this->Internals->AdjacencyHeap.clear();
  this->Internals->VertexLinkInfo.clear();
}

//----------------------------------------------------------------------------
unsigned long vtkVertexLinks::GetActualMemorySize()
{
  unsigned long size = 0;
  vtkIdType NumVertices = this->GetNumberOfVertices();
  for (vtkIdType i = 0; i < NumVertices; i++)
    {
    size += this->Internals->VertexLinkInfo[i].Allocated;
    }
  size *= sizeof(vtkIdType*);
  size += this->Internals->VertexLinkInfo.size()*sizeof(vtkVertexLinksInternals::vtkVertexLinkInfo);

  return (unsigned long) ceil((float)size/1024.0); //kilobytes
}

//----------------------------------------------------------------------------
void vtkVertexLinks::DeepCopy(vtkVertexLinks* src)
{
  this->Internals->VertexLinkInfo.clear();
  this->Internals->VertexLinkInfo.resize(src->Internals->VertexLinkInfo.size());
  this->Internals->AdjacencyHeap.clear();
  vtkIdType VertexLinkInfo = this->GetNumberOfVertices();
  for (vtkIdType i = 0; i < VertexLinkInfo; i++)
    {
    this->Internals->VertexLinkInfo[i] = src->Internals->VertexLinkInfo[i];
    this->Internals->VertexLinkInfo[i].Index = this->Internals->AdjacencyHeap.grab(this->Internals->VertexLinkInfo[i].Allocated);
    for (int j = 0; j < src->Internals->VertexLinkInfo[i].Degree; j++)
      {
      this->Internals->AdjacencyHeap[this->Internals->VertexLinkInfo[i].Index + j] = 
        src->Internals->AdjacencyHeap[src->Internals->VertexLinkInfo[i].Index + j];
      }
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkVertexLinks::GetDegree(vtkIdType vertex)
{
  return this->Internals->VertexLinkInfo[vertex].Degree;
}

//----------------------------------------------------------------------------
void vtkVertexLinks::GetAdjacent(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges)
{
  nedges = this->Internals->VertexLinkInfo[vertex].Degree;
  edges = this->Internals->AdjacencyHeap.pointer(this->Internals->VertexLinkInfo[vertex].Index);
}

//----------------------------------------------------------------------------
vtkIdType vtkVertexLinks::GetOutDegree(vtkIdType vertex)
{
  return this->Internals->VertexLinkInfo[vertex].Degree - this->Internals->VertexLinkInfo[vertex].InDegree;
}

//----------------------------------------------------------------------------
void vtkVertexLinks::GetOutAdjacent(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges)
{
  nedges = this->GetOutDegree(vertex);
  edges = this->Internals->AdjacencyHeap.pointer(this->Internals->VertexLinkInfo[vertex].Index + this->Internals->VertexLinkInfo[vertex].InDegree);
}

//----------------------------------------------------------------------------
vtkIdType vtkVertexLinks::GetInDegree(vtkIdType vertex)
{
  return this->Internals->VertexLinkInfo[vertex].InDegree;
}

//----------------------------------------------------------------------------
void vtkVertexLinks::GetInAdjacent(vtkIdType vertex, vtkIdType& nedges, const vtkIdType*& edges)
{
  nedges = this->GetInDegree(vertex);
  edges = this->Internals->AdjacencyHeap.pointer(this->Internals->VertexLinkInfo[vertex].Index);
}

//----------------------------------------------------------------------------
vtkIdType vtkVertexLinks::GetNumberOfVertices()
{
  return this->Internals->VertexLinkInfo.size();
}

//----------------------------------------------------------------------------
void vtkVertexLinks::ResizeVertexList(vtkIdType vertex, vtkIdType size)
{
  vtkIdType curSize = this->Internals->VertexLinkInfo[vertex].Allocated;
  //cout << "resizing VertexLinkInfo[" << vertex << "] from " << curSize << " to " << size << endl;
  if (size == curSize)
    {
    return;
    }
  if (size < curSize)
    {
    if (size == 0)
      {
      this->Internals->AdjacencyHeap.free(this->Internals->VertexLinkInfo[vertex].Index, 
                                          this->Internals->VertexLinkInfo[vertex].Allocated);
      this->Internals->VertexLinkInfo[vertex].Index = -1;
      this->Internals->VertexLinkInfo[vertex].Allocated = 0;
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
    vtkIdType arrIndex = this->Internals->AdjacencyHeap.grab(size);
    vtkIdType* arr = this->Internals->AdjacencyHeap.pointer(arrIndex);
    if (this->Internals->VertexLinkInfo[vertex].Index != -1)
      {
      vtkIdType* oldArr = this->Internals->AdjacencyHeap.pointer(this->Internals->VertexLinkInfo[vertex].Index);
      memcpy(arr, oldArr, curSize*sizeof(vtkIdType));
      this->Internals->AdjacencyHeap.free(this->Internals->VertexLinkInfo[vertex].Index, 
                                          this->Internals->VertexLinkInfo[vertex].Allocated);
      }
    this->Internals->VertexLinkInfo[vertex].Index = arrIndex;
    this->Internals->VertexLinkInfo[vertex].Allocated = size;
    }
}


vtkIdType vtkVertexLinks::AddVertex()
{
  this->Internals->VertexLinkInfo.push_back(vtkVertexLinksInternals::vtkVertexLinkInfo());
  return this->GetNumberOfVertices() - 1;
}

vtkIdType vtkVertexLinks::RemoveVertex(vtkIdType vertex)
{
  if (this->Internals->VertexLinkInfo[vertex].Allocated > 0)
    {
    this->Internals->AdjacencyHeap.free(
      this->Internals->VertexLinkInfo[vertex].Index, 
      this->Internals->VertexLinkInfo[vertex].Allocated);
    }
  vtkIdType movedVertex = this->GetNumberOfVertices() - 1;
  this->Internals->VertexLinkInfo[vertex] = 
    this->Internals->VertexLinkInfo[movedVertex];
  this->Internals->VertexLinkInfo.resize(this->Internals->VertexLinkInfo.size() - 1);
  return movedVertex;
}

void vtkVertexLinks::AddInAdjacent(vtkIdType vertex, vtkIdType adj)
{
  this->ResizeVertexList(vertex, this->GetDegree(vertex) + 1);
  vtkIdType adjacent = this->Internals->VertexLinkInfo[vertex].Index;
  this->Internals->AdjacencyHeap[adjacent + this->GetDegree(vertex)] =
    this->Internals->AdjacencyHeap[adjacent + this->GetInDegree(vertex)];
  this->Internals->AdjacencyHeap[adjacent + this->GetInDegree(vertex)] = adj;
  this->Internals->VertexLinkInfo[vertex].Degree++;
  this->Internals->VertexLinkInfo[vertex].InDegree++;
}

void vtkVertexLinks::AddOutAdjacent(vtkIdType vertex, vtkIdType adj)
{
  this->ResizeVertexList(vertex, this->GetDegree(vertex) + 1);
  vtkIdType index = this->Internals->VertexLinkInfo[vertex].Index;
  this->Internals->AdjacencyHeap[index + this->GetDegree(vertex)] = adj;
  this->Internals->VertexLinkInfo[vertex].Degree++;
}

void vtkVertexLinks::RemoveInAdjacent(vtkIdType vertex, vtkIdType adj)
{
  vtkIdType adjacent = this->Internals->VertexLinkInfo[vertex].Index;
  for (vtkIdType e = 0; e < this->GetInDegree(vertex); e++)
    {
    if (this->Internals->AdjacencyHeap[adjacent + e] == adj)
      {
      this->Internals->AdjacencyHeap[adjacent + e] = 
        this->Internals->AdjacencyHeap[adjacent + this->GetInDegree(vertex) - 1];
      this->Internals->AdjacencyHeap[adjacent + this->GetInDegree(vertex) - 1] = 
        this->Internals->AdjacencyHeap[adjacent + this->GetDegree(vertex) - 1];
      this->Internals->VertexLinkInfo[vertex].Degree--;
      this->Internals->VertexLinkInfo[vertex].InDegree--;
      break;
      }
    }
}

void vtkVertexLinks::RemoveOutAdjacent(vtkIdType vertex, vtkIdType adj)
{
  vtkIdType index = this->Internals->VertexLinkInfo[vertex].Index;
  for (vtkIdType e = this->GetInDegree(vertex); e < this->GetDegree(vertex); e++)
    {
    if (this->Internals->AdjacencyHeap[index + e] == adj)
      {
      this->Internals->AdjacencyHeap[index + e] = 
        this->Internals->AdjacencyHeap[index + this->GetDegree(vertex) - 1];
      this->Internals->VertexLinkInfo[vertex].Degree--;
      break;
      }
    }
}

void vtkVertexLinks::RemoveOutAdjacentShift(vtkIdType vertex, vtkIdType adj)
{
  vtkIdType index = this->Internals->VertexLinkInfo[vertex].Index;
  for (vtkIdType e = this->GetInDegree(vertex); e < this->GetDegree(vertex); e++)
    {
    if (this->Internals->AdjacencyHeap[index + e] == adj)
      {
      if (e < this->GetDegree(vertex) - 1)
        {
        vtkIdType* fromPtr = this->Internals->AdjacencyHeap.pointer(index + e + 1);
        vtkIdType* toPtr = this->Internals->AdjacencyHeap.pointer(index + e);
        int size = this->GetDegree(vertex) - e - 1;
        memmove(toPtr, fromPtr, size*sizeof(vtkIdType));
        }
      this->Internals->VertexLinkInfo[vertex].Degree--;
      break;
      }
    }
}

vtkIdType vtkVertexLinks::GetOutAdjacent(vtkIdType vertex, vtkIdType index)
{
  return this->Internals->AdjacencyHeap[this->Internals->VertexLinkInfo[vertex].Index + 
                              this->Internals->VertexLinkInfo[vertex].InDegree + index];
}

vtkIdType vtkVertexLinks::GetInAdjacent(vtkIdType vertex, vtkIdType index)
{
  return this->Internals->AdjacencyHeap[this->Internals->VertexLinkInfo[vertex].Index + index];
}

void vtkVertexLinks::SetOutAdjacent(vtkIdType vertex, vtkIdType index, vtkIdType value)
{
  this->Internals->AdjacencyHeap[this->Internals->VertexLinkInfo[vertex].Index + this->Internals->VertexLinkInfo[vertex].InDegree + index] =
    value;
}

void vtkVertexLinks::SetInAdjacent(vtkIdType vertex, vtkIdType index, vtkIdType value)
{
  this->Internals->AdjacencyHeap[this->Internals->VertexLinkInfo[vertex].Index + index] =
    value;
}

