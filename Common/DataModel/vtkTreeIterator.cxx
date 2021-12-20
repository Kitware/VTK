/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTreeIterator.h"

#include "vtkTree.h"

vtkTreeIterator::vtkTreeIterator()
{
  this->Tree = nullptr;
  this->StartVertex = -1;
  this->NextId = -1;
}

vtkTreeIterator::~vtkTreeIterator()
{
  if (this->Tree)
  {
    this->Tree->UnRegister(this);
    this->Tree = nullptr;
  }
}

void vtkTreeIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Tree: " << this->Tree << endl;
  os << indent << "StartVertex: " << this->StartVertex << endl;
  os << indent << "NextId: " << this->NextId << endl;
}

void vtkTreeIterator::SetTree(vtkTree* tree)
{
  bool needs_init = this->Tree != tree;
  vtkSetObjectBodyMacro(Tree, vtkTree, tree);
  if (needs_init)
  {
    this->StartVertex = -1;
    this->Initialize();
  }
}

void vtkTreeIterator::SetStartVertex(vtkIdType vertex)
{
  if (this->StartVertex != vertex)
  {
    this->StartVertex = vertex;
    this->Initialize();
    this->Modified();
  }
}

vtkIdType vtkTreeIterator::Next()
{
  vtkIdType last = this->NextId;
  if (last != -1)
  {
    this->NextId = this->NextInternal();
  }
  return last;
}

bool vtkTreeIterator::HasNext()
{
  return this->NextId != -1;
}

void vtkTreeIterator::Restart()
{
  this->Initialize();
}
