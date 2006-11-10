/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphDFSIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGraphDFSIterator.h"
#include "vtkGraph.h"
#include "vtkTree.h"
#include "vtkObjectFactory.h"
#include "vtkIntArray.h"
#include "vtkIdList.h"

#include <vtkstd/stack>
using vtkstd::stack;

struct vtkGraphDFSIteratorPosition
{
  vtkGraphDFSIteratorPosition(vtkIdType node, vtkIdType index)
    : Node(node), Index(index) { }
  vtkIdType Node;
  vtkIdType Index; // How far along we are in the node's edge array
};

class vtkGraphDFSIteratorInternals
{
public:
  stack<vtkGraphDFSIteratorPosition> Stack;
};

vtkCxxRevisionMacro(vtkGraphDFSIterator, "1.2");
vtkStandardNewMacro(vtkGraphDFSIterator);

vtkGraphDFSIterator::vtkGraphDFSIterator()
{
  this->Internals = new vtkGraphDFSIteratorInternals();
  this->Graph = NULL;
  this->Color = vtkIntArray::New();
  this->StartNode = -1;
  this->Mode = 0;
}

vtkGraphDFSIterator::~vtkGraphDFSIterator()
{
  if (this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
  if (this->Graph)
    {
    this->Graph->Delete();
    this->Graph = NULL;
    }
  if (this->Color)
    {
    this->Color->Delete();
    this->Color = NULL;
    }
}

void vtkGraphDFSIterator::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Mode: " << this->Mode << endl;
  os << indent << "StartNode: " << this->StartNode << endl;
}

void vtkGraphDFSIterator::Initialize()
{
  if (this->Graph == NULL)
    {
    return;
    }
  // Set all colors to white
  this->Color->Resize(this->Graph->GetNumberOfNodes());
  for (vtkIdType i = 0; i < this->Graph->GetNumberOfNodes(); i++)
    {
    this->Color->SetValue(i, this->WHITE);
    }
  if (this->StartNode < 0)
    {
    this->StartNode = 0;
    }
  this->CurRoot = this->StartNode;
  while (this->Internals->Stack.size())
    {
    this->Internals->Stack.pop();
    }
  this->NumBlack = 0;

  // Find the first item
  if (this->Graph->GetNumberOfNodes() > 0)
    {
    this->NextId = this->NextInternal();
    }
  else
    {
    this->NextId = -1;
    }
}

void vtkGraphDFSIterator::SetGraph(vtkGraph* graph)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this
                << "): setting Graph to " << graph );
  if (this->Graph != graph)
    {
    vtkGraph* temp = this->Graph;
    this->Graph = graph;
    if (this->Graph != NULL) { this->Graph->Register(this); }
    if (temp != NULL)
      {
      temp->UnRegister(this);
      }
    this->Initialize();
    this->Modified();
    }
}

void vtkGraphDFSIterator::SetStartNode(vtkIdType node)
{
  if (this->StartNode != node)
    {
    this->StartNode = node;
    this->Initialize();
    this->Modified();
    }
}

void vtkGraphDFSIterator::SetMode(int mode)
{
  if (this->Mode != mode)
    {
    this->Mode = mode;
    this->Initialize();
    this->Modified();
    }
}

vtkIdType vtkGraphDFSIterator::Next()
{
  vtkIdType last = this->NextId;
  this->NextId = this->NextInternal();
  return last;
}

vtkIdType vtkGraphDFSIterator::NextInternal()
{
  while (this->NumBlack < this->Graph->GetNumberOfNodes())
    {
    while (this->Internals->Stack.size() > 0)
      {
      // Pop the current position off the stack
      vtkGraphDFSIteratorPosition pos = this->Internals->Stack.top();
      this->Internals->Stack.pop();
      //cout << "popped " << pos.Node << "," << pos.Index << " off the stack" << endl;

      vtkIdType narcs;
      const vtkIdType* arcs;
      this->Graph->GetOutArcs(pos.Node, narcs, arcs);
      while (pos.Index < narcs && 
             this->Color->GetValue(this->Graph->GetOppositeNode(arcs[pos.Index], pos.Node)) != this->WHITE)
        {
        pos.Index++;
        }
      if (pos.Index == narcs)
        {
        //cout << "DFS coloring " << pos.Node << " black" << endl;
        // Done with this node; make it black and leave it off the stack
        this->Color->SetValue(pos.Node, this->BLACK);
        this->NumBlack++;
        if (this->Mode == this->FINISH)
          {
          //cout << "DFS finished " << pos.Node << endl;
          return pos.Node;
          }
        }
      else
        {
        // Not done with this node; put it back on the stack
        this->Internals->Stack.push(pos);

        // Found a white node; make it gray, add it to the stack
        vtkIdType found = this->Graph->GetOppositeNode(arcs[pos.Index], pos.Node);
        //cout << "DFS coloring " << found << " gray (adjacency)" << endl;
        this->Color->SetValue(found, this->GRAY);
        this->Internals->Stack.push(vtkGraphDFSIteratorPosition(found, 0));
        if (this->Mode == this->DISCOVER)
          {
          //cout << "DFS adjacent discovery " << found << endl;
          return found;
          }
        }
      }

    // Done with this component, so find a white node and start a new search
    for (; this->CurRoot < this->Graph->GetNumberOfNodes(); this->CurRoot++)
      {
      if (this->Color->GetValue(this->CurRoot) == this->WHITE)
        {
        // Found a new component; make it gray, put it on the stack
        //cout << "DFS coloring " << this->CurRoot << " gray (new component)" << endl;
        this->Internals->Stack.push(vtkGraphDFSIteratorPosition(this->CurRoot, 0));
        this->Color->SetValue(this->CurRoot, this->GRAY);
        if (this->Mode == this->DISCOVER)
          {
          //cout << "DFS new component discovery " << this->CurRoot << endl;
          return this->CurRoot;
          }
        break;
        }
      }
    }
  //cout << "DFS no more!" << endl;
  return -1;
}

bool vtkGraphDFSIterator::HasNext()
{
  return this->NextId != -1;
}
