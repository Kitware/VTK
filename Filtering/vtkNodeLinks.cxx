/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNodeLinks.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNodeLinks.h"
#include "vtkObjectFactory.h"
#include "stdlib.h"

#include <freerange/freerange>
#include <vtkstd/vector>
using vtkstd::vector;

struct vtkNodeLinksInternals
{
  struct vtkNode {
    vtkNode() : InDegree(0), Degree(0), Allocated(0), Adjacent(-1) { }
    vtkIdType InDegree;
    vtkIdType Degree;
    vtkIdType Allocated;
    vtkIdType Adjacent;
  };

  vector<vtkNode> Nodes;
  freerange<vtkIdType, vtkIdType, -1> FreeRange;
};


vtkCxxRevisionMacro(vtkNodeLinks, "1.1");
vtkStandardNewMacro(vtkNodeLinks);

//----------------------------------------------------------------------------
vtkNodeLinks::vtkNodeLinks()
{
  this->Internals = new vtkNodeLinksInternals();
}

//----------------------------------------------------------------------------
vtkNodeLinks::~vtkNodeLinks()
{
  if (Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkNodeLinks::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);  
  os << indent << "Nodes Size: " << this->Internals->Nodes.size() << endl;
}

//----------------------------------------------------------------------------
void vtkNodeLinks::Reset()
{
  this->Internals->FreeRange.clear();
  this->Internals->Nodes.clear();
}

//----------------------------------------------------------------------------
unsigned long vtkNodeLinks::GetActualMemorySize()
{
  unsigned long size = 0;
  vtkIdType nodes = this->GetNumberOfNodes();
  for (vtkIdType i = 0; i < nodes; i++)
    {
    size += this->Internals->Nodes[i].Allocated;
    }
  size *= sizeof(vtkIdType*);
  size += this->Internals->Nodes.size()*sizeof(vtkNodeLinksInternals::vtkNode);

  return (unsigned long) ceil((float)size/1000.0); //kilobytes
}

//----------------------------------------------------------------------------
void vtkNodeLinks::DeepCopy(vtkNodeLinks* src)
{
  this->Internals->Nodes.clear();
  this->Internals->Nodes.resize(src->Internals->Nodes.size());
  this->Internals->FreeRange.clear();
  vtkIdType nodes = this->GetNumberOfNodes();
  for (vtkIdType i = 0; i < nodes; i++)
    {
    this->Internals->Nodes[i] = src->Internals->Nodes[i];
    this->Internals->Nodes[i].Adjacent = this->Internals->FreeRange.grab(this->Internals->Nodes[i].Allocated);
    for (int j = 0; j < src->Internals->Nodes[i].Degree; j++)
      {
      this->Internals->FreeRange[this->Internals->Nodes[i].Adjacent + j] = 
        src->Internals->FreeRange[src->Internals->Nodes[i].Adjacent + j];
      }
    }
}

//----------------------------------------------------------------------------
vtkIdType vtkNodeLinks::GetDegree(vtkIdType node)
{
  return this->Internals->Nodes[node].Degree;
}

//----------------------------------------------------------------------------
void vtkNodeLinks::GetAdjacent(vtkIdType node, vtkIdType& narcs, const vtkIdType*& arcs)
{
  narcs = this->Internals->Nodes[node].Degree;
  arcs = this->Internals->FreeRange.pointer(this->Internals->Nodes[node].Adjacent);
}

//----------------------------------------------------------------------------
vtkIdType vtkNodeLinks::GetOutDegree(vtkIdType node)
{
  return this->Internals->Nodes[node].Degree - this->Internals->Nodes[node].InDegree;
}

//----------------------------------------------------------------------------
void vtkNodeLinks::GetOutAdjacent(vtkIdType node, vtkIdType& narcs, const vtkIdType*& arcs)
{
  narcs = this->GetOutDegree(node);
  arcs = this->Internals->FreeRange.pointer(this->Internals->Nodes[node].Adjacent + this->Internals->Nodes[node].InDegree);
}

//----------------------------------------------------------------------------
vtkIdType vtkNodeLinks::GetInDegree(vtkIdType node)
{
  return this->Internals->Nodes[node].InDegree;
}

//----------------------------------------------------------------------------
void vtkNodeLinks::GetInAdjacent(vtkIdType node, vtkIdType& narcs, const vtkIdType*& arcs)
{
  narcs = this->GetInDegree(node);
  arcs = this->Internals->FreeRange.pointer(this->Internals->Nodes[node].Adjacent);
}

//----------------------------------------------------------------------------
vtkIdType vtkNodeLinks::GetNumberOfNodes()
{
  return this->Internals->Nodes.size();
}

//----------------------------------------------------------------------------
void vtkNodeLinks::ResizeNodeList(vtkIdType node, vtkIdType size)
{
  vtkIdType curSize = this->Internals->Nodes[node].Allocated;
  //cout << "resizing Nodes[" << node << "] from " << curSize << " to " << size << endl;
  if (size == curSize)
    {
    return;
    }
  if (size < curSize)
    {
    if (size == 0)
      {
      this->Internals->FreeRange.free(this->Internals->Nodes[node].Adjacent, this->Internals->Nodes[node].Allocated);
      this->Internals->Nodes[node].Adjacent = -1;
      this->Internals->Nodes[node].Allocated = 0;
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
    if (this->Internals->Nodes[node].Adjacent != -1)
      {
      vtkIdType* oldArr = this->Internals->FreeRange.pointer(this->Internals->Nodes[node].Adjacent);
      memcpy(arr, oldArr, curSize*sizeof(vtkIdType));
      this->Internals->FreeRange.free(this->Internals->Nodes[node].Adjacent, this->Internals->Nodes[node].Allocated);
      }
    this->Internals->Nodes[node].Adjacent = arrIndex;
    this->Internals->Nodes[node].Allocated = size;
    }
}


vtkIdType vtkNodeLinks::AddNode()
{
  this->Internals->Nodes.push_back(vtkNodeLinksInternals::vtkNode());
  return this->GetNumberOfNodes() - 1;
}

vtkIdType vtkNodeLinks::RemoveNode(vtkIdType node)
{
  if (this->Internals->Nodes[node].Allocated > 0)
    {
    this->Internals->FreeRange.free(
      this->Internals->Nodes[node].Adjacent, 
      this->Internals->Nodes[node].Allocated);
    }
  vtkIdType movedNode = this->GetNumberOfNodes() - 1;
  this->Internals->Nodes[node] = 
    this->Internals->Nodes[movedNode];
  this->Internals->Nodes.resize(this->Internals->Nodes.size() - 1);
  return movedNode;
}

void vtkNodeLinks::AddInAdjacent(vtkIdType node, vtkIdType adj)
{
  this->ResizeNodeList(node, this->GetDegree(node) + 1);
  vtkIdType adjacent = this->Internals->Nodes[node].Adjacent;
  this->Internals->FreeRange[adjacent + this->GetDegree(node)] =
    this->Internals->FreeRange[adjacent + this->GetInDegree(node)];
  this->Internals->FreeRange[adjacent + this->GetInDegree(node)] = adj;
  this->Internals->Nodes[node].Degree++;
  this->Internals->Nodes[node].InDegree++;
}

void vtkNodeLinks::AddOutAdjacent(vtkIdType node, vtkIdType adj)
{
  this->ResizeNodeList(node, this->GetDegree(node) + 1);
  vtkIdType adjacent = this->Internals->Nodes[node].Adjacent;
  this->Internals->FreeRange[adjacent + this->GetDegree(node)] = adj;
  this->Internals->Nodes[node].Degree++;
}

void vtkNodeLinks::RemoveInAdjacent(vtkIdType node, vtkIdType adj)
{
  vtkIdType adjacent = this->Internals->Nodes[node].Adjacent;
  for (vtkIdType e = 0; e < this->GetInDegree(node); e++)
    {
    if (this->Internals->FreeRange[adjacent + e] == adj)
      {
      this->Internals->FreeRange[adjacent + e] = 
        this->Internals->FreeRange[adjacent + this->GetInDegree(node) - 1];
      this->Internals->FreeRange[adjacent + this->GetInDegree(node) - 1] = 
        this->Internals->FreeRange[adjacent + this->GetDegree(node) - 1];
      this->Internals->Nodes[node].Degree--;
      this->Internals->Nodes[node].InDegree--;
      break;
      }
    }
}

void vtkNodeLinks::RemoveOutAdjacent(vtkIdType node, vtkIdType adj)
{
  vtkIdType adjacent = this->Internals->Nodes[node].Adjacent;
  for (vtkIdType e = this->GetInDegree(node); e < this->GetDegree(node); e++)
    {
    if (this->Internals->FreeRange[adjacent + e] == adj)
      {
      this->Internals->FreeRange[adjacent + e] = 
        this->Internals->FreeRange[adjacent + this->GetDegree(node) - 1];
      this->Internals->Nodes[node].Degree--;
      break;
      }
    }
}

vtkIdType vtkNodeLinks::GetOutAdjacent(vtkIdType node, vtkIdType index)
{
  return this->Internals->FreeRange[this->Internals->Nodes[node].Adjacent + this->Internals->Nodes[node].InDegree + index];
}

vtkIdType vtkNodeLinks::GetInAdjacent(vtkIdType node, vtkIdType index)
{
  return this->Internals->FreeRange[this->Internals->Nodes[node].Adjacent + index];
}

void vtkNodeLinks::SetOutAdjacent(vtkIdType node, vtkIdType index, vtkIdType value)
{
  this->Internals->FreeRange[this->Internals->Nodes[node].Adjacent + this->Internals->Nodes[node].InDegree + index] =
    value;
}

void vtkNodeLinks::SetInAdjacent(vtkIdType node, vtkIdType index, vtkIdType value)
{
  this->Internals->FreeRange[this->Internals->Nodes[node].Adjacent + index] =
    value;
}

