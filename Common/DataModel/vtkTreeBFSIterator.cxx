/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeBFSIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeBFSIterator.h"

#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkTree.h"

#include <queue>
using std::queue;

class vtkTreeBFSIteratorInternals
{
public:
  queue<vtkIdType> Queue;
};

vtkStandardNewMacro(vtkTreeBFSIterator);

vtkTreeBFSIterator::vtkTreeBFSIterator()
{
  this->Internals = new vtkTreeBFSIteratorInternals();
  this->Color = vtkIntArray::New();
}

vtkTreeBFSIterator::~vtkTreeBFSIterator()
{
  delete this->Internals;
  this->Internals = NULL;

  if (this->Color)
  {
    this->Color->Delete();
    this->Color = NULL;
  }
}

void vtkTreeBFSIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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
