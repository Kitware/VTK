/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHeap.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHeap.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkHeap, "1.7");
vtkStandardNewMacro(vtkHeap);

vtkHeap::vtkHeap()
{
  this->First = 0;
  this->Last = 0;
  this->Current = 0;
  this->NumberOfAllocations = 0;
}

vtkHeap::~vtkHeap()
{
  this->CleanAll();
}

void vtkHeap::Add(vtkHeapNode* node)
{
  node->Next = 0;
  if (!this->Last)
    {
    this->Last = node;
    this->First = node;
    return;
    }
  this->Last->Next = node;
  this->Last = node;
}

void vtkHeap::CleanAll()
{
  this->Current = this->First;
  if (!this->Current) { return; }
  while (this->DeleteAndNext());
}

vtkHeapNode* vtkHeap::DeleteAndNext()
{
  if (this->Current)
    {
    vtkHeapNode* tmp = this->Current;
    this->Current = this->Current->Next;
    if (tmp->Ptr)
      {
      free(tmp->Ptr);
      }
    delete tmp;
    return this->Current;
    }
  else
    {
    return 0;
    }
}

void* vtkHeap::AllocateMemory(size_t n)
{
  this->NumberOfAllocations++;
  
  vtkHeapNode* node = new vtkHeapNode;
  node->Ptr = malloc(n);
  this->Add(node);
  return node->Ptr;
}

char* vtkHeap::StringDup(const char* str)
{
  this->NumberOfAllocations++;

  vtkHeapNode* node = new vtkHeapNode;
#ifdef _WIN32_WCE
  node->Ptr = _strdup(str);
#else
  node->Ptr = strdup(str);
#endif
  this->Add(node);
  return static_cast<char*>(node->Ptr);
}

void vtkHeap::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of Allocations: " << this->NumberOfAllocations << "\n";
}

