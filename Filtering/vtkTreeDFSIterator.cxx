/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeDFSIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkTreeDFSIterator.h"
#include "vtkTree.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"

#include <vtkstd/stack>
using vtkstd::stack;

struct vtkTreeDFSIteratorPosition
{
  vtkTreeDFSIteratorPosition(vtkIdType vertex, vtkIdType index)
    : Vertex(vertex), Index(index) { }
  vtkIdType Vertex;
  vtkIdType Index; // How far along we are in the vertex's edge array
};

class vtkTreeDFSIteratorInternals
{
public:
  stack<vtkTreeDFSIteratorPosition> Stack;
};

vtkStandardNewMacro(vtkTreeDFSIterator);

vtkTreeDFSIterator::vtkTreeDFSIterator()
{
  this->Internals = new vtkTreeDFSIteratorInternals();
  this->Tree = NULL;
  this->Color = vtkIntArray::New();
  this->StartVertex = -1;
  this->Mode = 0;
}

vtkTreeDFSIterator::~vtkTreeDFSIterator()
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

void vtkTreeDFSIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode: " << this->Mode << endl;
  os << indent << "StartVertex: " << this->StartVertex << endl;
}

void vtkTreeDFSIterator::Initialize()
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
  this->CurRoot = this->StartVertex;
  while (this->Internals->Stack.size())
    {
    this->Internals->Stack.pop();
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

void vtkTreeDFSIterator::SetTree(vtkTree* tree)
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

void vtkTreeDFSIterator::SetStartVertex(vtkIdType vertex)
{
  if (this->StartVertex != vertex)
    {
    this->StartVertex = vertex;
    this->Initialize();
    this->Modified();
    }
}

void vtkTreeDFSIterator::SetMode(int mode)
{
  if (this->Mode != mode)
    {
    this->Mode = mode;
    this->Initialize();
    this->Modified();
    }
}

vtkIdType vtkTreeDFSIterator::Next()
{
  vtkIdType last = this->NextId;
  this->NextId = this->NextInternal();
  return last;
}

vtkIdType vtkTreeDFSIterator::NextInternal()
{
  while (this->Color->GetValue(this->StartVertex) != this->BLACK)
    {
    while (this->Internals->Stack.size() > 0)
      {
      // Pop the current position off the stack
      vtkTreeDFSIteratorPosition pos = this->Internals->Stack.top();
      this->Internals->Stack.pop();
      //cout << "popped " << pos.Vertex << "," << pos.Index << " off the stack" << endl;

      vtkIdType nchildren = this->Tree->GetNumberOfChildren(pos.Vertex);
      while (pos.Index < nchildren && 
             this->Color->GetValue(this->Tree->GetChild(pos.Vertex, pos.Index)) != this->WHITE)
        {
        pos.Index++;
        }
      if (pos.Index == nchildren)
        {
        //cout << "DFS coloring " << pos.Vertex << " black" << endl;
        // Done with this vertex; make it black and leave it off the stack
        this->Color->SetValue(pos.Vertex, this->BLACK);
        if (this->Mode == this->FINISH)
          {
          //cout << "DFS finished " << pos.Vertex << endl;
          return pos.Vertex;
          }
        // Done with the start vertex, so we are totally done!
        if (pos.Vertex == this->StartVertex)
          {
          return -1;
          }
        }
      else
        {
        // Not done with this vertex; put it back on the stack
        this->Internals->Stack.push(pos);

        // Found a white vertex; make it gray, add it to the stack
        vtkIdType found = this->Tree->GetChild(pos.Vertex, pos.Index);
        //cout << "DFS coloring " << found << " gray (adjacency)" << endl;
        this->Color->SetValue(found, this->GRAY);
        this->Internals->Stack.push(vtkTreeDFSIteratorPosition(found, 0));
        if (this->Mode == this->DISCOVER)
          {
          //cout << "DFS adjacent discovery " << found << endl;
          return found;
          }
        }
      }

    // Done with this component, so find a white vertex and start a new seedgeh
    if (this->Color->GetValue(this->StartVertex) != this->BLACK)
      {
      while (true)
        {
        if (this->Color->GetValue(this->CurRoot) == this->WHITE)
          {
          // Found a new component; make it gray, put it on the stack
          //cerr << "DFS coloring " << this->CurRoot << " gray (new component)" << endl;
          this->Internals->Stack.push(vtkTreeDFSIteratorPosition(this->CurRoot, 0));
          this->Color->SetValue(this->CurRoot, this->GRAY);
          if (this->Mode == this->DISCOVER)
            {
            //cerr << "DFS new component discovery " << this->CurRoot << endl;
            return this->CurRoot;
            }
          break;
          }
        else if (this->Color->GetValue(this->CurRoot) == this->GRAY)
          {
          vtkErrorMacro("There should be no gray vertices in the graph when starting a new component.");
          }
        this->CurRoot = (this->CurRoot + 1) % this->Tree->GetNumberOfVertices();
        }
      }
    }
  //cout << "DFS no more!" << endl;
  return -1;
}

bool vtkTreeDFSIterator::HasNext()
{
  return this->NextId != -1;
}
