/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHeap.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tom Citriniti who implemented and contributed this class


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkHeap.h"
#include "vtkObjectFactory.h"

vtkHeap* vtkHeap::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkHeap");
  if(ret)
    {
    return (vtkHeap*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkHeap;
}

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

char* vtkHeap::StrDup(const char* str)
{
  this->NumberOfAllocations++;

  vtkHeapNode* node = new vtkHeapNode;
  node->Ptr = strdup(str);
  this->Add(node);
  return static_cast<char*>(node->Ptr);
}


