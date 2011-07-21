/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkTreeBFSIterator.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeBFSIterator.h"
#include "vtkTree.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"

#include <vtkstd/queue>
using vtkstd::queue;

class vtkTreeBFSIteratorInternals
{
public:
  queue<vtkIdType> Queue;
};

vtkStandardNewMacro(vtkTreeBFSIterator);

vtkTreeBFSIterator::vtkTreeBFSIterator()
{
  this->Internals = new vtkTreeBFSIteratorInternals();
  this->Tree = NULL;
  this->Color = vtkIntArray::New();
  this->StartVertex = -1;
  this->Mode = 0;
}

vtkTreeBFSIterator::~vtkTreeBFSIterator()
{
  if (this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
  if (this->Tree)
    {
    this->Tree->Delete();
    this->Tree = NULL;
    }
  if (this->Color)
    {
    this->Color->Delete();
    this->Color = NULL;
    }
}

void vtkTreeBFSIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode: " << this->Mode << endl;
  os << indent << "StartVertex: " << this->StartVertex << endl;
}

void vtkTreeBFSIterator::Initialize()
{
  if (this->Tree == NULL)
    {
    return;
    }
  // Set all colors to white
  this->Color->Resize(this->Tree->GetNumberOfVertices());
  for (vtkIdType i = 0; i < this->Tree->GetNumberOfVertices(); i++)
    {
    this->Color->SetValue(i, this->WHITE);
    }
  if (this->StartVertex < 0)
    {
    this->StartVertex = this->Tree->GetRoot();
    }
  while (this->Internals->Queue.size())
    {
    this->Internals->Queue.pop();
    }

  // Find the first item
  if (this->Tree->GetNumberOfVertices() > 0)
    {
    this->NextId = this->NextInternal();
    }
  else
    {
    this->NextId = -1;
    }
}

void vtkTreeBFSIterator::SetTree(vtkTree* tree)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting Tree to " << tree );
  if (this->Tree != tree)
    {
    vtkTree* temp = this->Tree;
    this->Tree = tree;
    if (this->Tree != NULL) { this->Tree->Register(this); }
    if (temp != NULL)
      {
      temp->UnRegister(this);
      }
    this->StartVertex = -1;
    this->Initialize();
    this->Modified();
    }
}

void vtkTreeBFSIterator::SetStartVertex(vtkIdType vertex)
{
  if (this->StartVertex != vertex)
    {
    this->StartVertex = vertex;
    this->Initialize();
    this->Modified();
    }
}

void vtkTreeBFSIterator::SetMode(int mode)
{
  if (this->Mode != mode)
    {
    this->Mode = mode;
    this->Initialize();
    this->Modified();
    }
}

vtkIdType vtkTreeBFSIterator::Next()
{
  vtkIdType last = this->NextId;
  if(last != -1)
    {
    this->NextId = this->NextInternal();
    }
  return last;
}

vtkIdType vtkTreeBFSIterator::NextInternal()
{
  if(this->Color->GetValue(this->StartVertex) == this->WHITE)
    {
    this->Color->SetValue(this->StartVertex, this->GRAY);
    this->Internals->Queue.push(this->StartVertex);
    }

  while (this->Internals->Queue.size() > 0)
    {
    vtkIdType currentId = this->Internals->Queue.front();
    this->Internals->Queue.pop();

    for(vtkIdType childNum = 0; childNum < this->Tree->GetNumberOfChildren(currentId); childNum++)
      {
      vtkIdType childId = this->Tree->GetChild(currentId, childNum);
      if(this->Color->GetValue(childId) == this->WHITE)
        {
        // Found a white vertex; make it gray, add it to the queue
        this->Color->SetValue(childId, this->GRAY);
        this->Internals->Queue.push(childId);
        }
      }

    this->Color->SetValue(currentId, this->BLACK);
    return currentId;
    }
  return -1;
}

bool vtkTreeBFSIterator::HasNext()
{
  return this->NextId != -1;
}
