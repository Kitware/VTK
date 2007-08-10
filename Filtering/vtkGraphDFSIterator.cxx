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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

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
  vtkGraphDFSIteratorPosition(vtkIdType vertex, vtkIdType index)
    : Vertex(vertex), Index(index) { }
  vtkIdType Vertex;
  vtkIdType Index; // How far along we are in the vertex's edge array
};

class vtkGraphDFSIteratorInternals
{
public:
  stack<vtkGraphDFSIteratorPosition> Stack;
};

vtkCxxRevisionMacro(vtkGraphDFSIterator, "1.5");
vtkStandardNewMacro(vtkGraphDFSIterator);

vtkGraphDFSIterator::vtkGraphDFSIterator()
{
  this->Internals = new vtkGraphDFSIteratorInternals();
  this->Graph = NULL;
  this->Color = vtkIntArray::New();
  this->StartVertex = 0;
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
  os << indent << "StartVertex: " << this->StartVertex << endl;
}

void vtkGraphDFSIterator::Initialize()
{
  if (this->Graph == NULL)
    {
    return;
    }
  // Set all colors to white
  this->Color->Resize(this->Graph->GetNumberOfVertices());
  for (vtkIdType i = 0; i < this->Graph->GetNumberOfVertices(); i++)
    {
    this->Color->SetValue(i, this->WHITE);
    }
  if (this->StartVertex < 0)
    {
    this->StartVertex = 0;
    }
  this->CurRoot = this->StartVertex;
  while (this->Internals->Stack.size())
    {
    this->Internals->Stack.pop();
    }
  this->NumBlack = 0;

  // Find the first item
  if (this->Graph->GetNumberOfVertices() > 0)
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

void vtkGraphDFSIterator::SetStartVertex(vtkIdType vertex)
{
  if (this->StartVertex != vertex)
    {
    this->StartVertex = vertex;
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
  while (this->NumBlack < this->Graph->GetNumberOfVertices())
    {
    while (this->Internals->Stack.size() > 0)
      {
      // Pop the current position off the stack
      vtkGraphDFSIteratorPosition pos = this->Internals->Stack.top();
      this->Internals->Stack.pop();
      //cerr << "popped " << pos.Vertex << "," << pos.Index << " off the stack" << endl;

      vtkIdType nedges;
      const vtkIdType* edges;
      this->Graph->GetOutEdges(pos.Vertex, nedges, edges);
      while (pos.Index < nedges && 
             this->Color->GetValue(this->Graph->GetOppositeVertex(edges[pos.Index], pos.Vertex)) != this->WHITE)
        {
        pos.Index++;
        }
      if (pos.Index == nedges)
        {
        //cerr << "DFS coloring " << pos.Vertex << " black" << endl;
        // Done with this vertex; make it black and leave it off the stack
        this->Color->SetValue(pos.Vertex, this->BLACK);
        this->NumBlack++;
        if (this->Mode == this->FINISH)
          {
          //cerr << "DFS finished " << pos.Vertex << endl;
          return pos.Vertex;
          }
        }
      else
        {
        // Not done with this vertex; put it back on the stack
        this->Internals->Stack.push(pos);

        // Found a white vertex; make it gray, add it to the stack
        vtkIdType found = this->Graph->GetOppositeVertex(edges[pos.Index], pos.Vertex);
        //cerr << "DFS coloring " << found << " gray (adjacency)" << endl;
        this->Color->SetValue(found, this->GRAY);
        this->Internals->Stack.push(vtkGraphDFSIteratorPosition(found, 0));
        if (this->Mode == this->DISCOVER)
          {
          //cerr << "DFS adjacent discovery " << found << endl;
          return found;
          }
        }
      }

    // Done with this component, so find a white vertex and start a new seedgeh
    if (this->NumBlack < this->Graph->GetNumberOfVertices())
      {
      while (true)
        {
        if (this->Color->GetValue(this->CurRoot) == this->WHITE)
          {
          // Found a new component; make it gray, put it on the stack
          //cerr << "DFS coloring " << this->CurRoot << " gray (new component)" << endl;
          this->Internals->Stack.push(vtkGraphDFSIteratorPosition(this->CurRoot, 0));
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
        this->CurRoot = (this->CurRoot + 1) % this->Graph->GetNumberOfVertices();
        }
      }
    }
  //cerr << "DFS no more!" << endl;
  return -1;
}

bool vtkGraphDFSIterator::HasNext()
{
  return this->NextId != -1;
}
