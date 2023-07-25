// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkTreeBFSIterator.h"

#include "vtkIntArray.h"
#include "vtkObjectFactory.h"
#include "vtkTree.h"

#include <queue>
using std::queue;

VTK_ABI_NAMESPACE_BEGIN
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
  this->Internals = nullptr;

  if (this->Color)
  {
    this->Color->Delete();
    this->Color = nullptr;
  }
}

void vtkTreeBFSIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void vtkTreeBFSIterator::Initialize()
{
  if (this->Tree == nullptr)
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
  while (!this->Internals->Queue.empty())
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
  if (this->Color->GetValue(this->StartVertex) == this->WHITE)
  {
    this->Color->SetValue(this->StartVertex, this->GRAY);
    this->Internals->Queue.push(this->StartVertex);
  }

  while (!this->Internals->Queue.empty())
  {
    vtkIdType currentId = this->Internals->Queue.front();
    this->Internals->Queue.pop();

    for (vtkIdType childNum = 0; childNum < this->Tree->GetNumberOfChildren(currentId); childNum++)
    {
      vtkIdType childId = this->Tree->GetChild(currentId, childNum);
      if (this->Color->GetValue(childId) == this->WHITE)
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
VTK_ABI_NAMESPACE_END
