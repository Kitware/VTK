/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutableDirectedGraph.cxx

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

#include "vtkMutableDirectedGraph.h"

#include "vtkGraphEdge.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
// class vtkMutableDirectedGraph
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMutableDirectedGraph, "1.1.4.2");
vtkStandardNewMacro(vtkMutableDirectedGraph);
//----------------------------------------------------------------------------
vtkMutableDirectedGraph::vtkMutableDirectedGraph()
{
  this->GraphEdge = vtkGraphEdge::New();
}

//----------------------------------------------------------------------------
vtkMutableDirectedGraph::~vtkMutableDirectedGraph()
{
  this->GraphEdge->Delete();
}

//----------------------------------------------------------------------------
vtkIdType vtkMutableDirectedGraph::AddVertex()
{
  return this->AddVertexInternal();
}

//----------------------------------------------------------------------------
vtkIdType vtkMutableDirectedGraph::AddVertex(vtkVariantArray *variantValueArr)
{
  return this->AddVertexInternal(variantValueArr);
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableDirectedGraph::AddEdge(vtkIdType u, vtkIdType v)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, true, &e);
  return e;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableDirectedGraph::AddEdge(vtkIdType u, vtkIdType v, vtkVariantArray *variantValueArr)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, true, &e, variantValueArr);
  return e;
}

//----------------------------------------------------------------------------
void vtkMutableDirectedGraph::AddEdge(vtkIdType u, vtkIdType v, vtkEdgeType *edge)
{
  this->AddEdgeInternal(u, v, true, edge);
}

//----------------------------------------------------------------------------
void vtkMutableDirectedGraph::AddEdge(vtkIdType u, vtkIdType v, vtkEdgeType *edge, vtkVariantArray *variantValueArr)
{
  this->AddEdgeInternal(u, v, true, edge, variantValueArr);
}

//----------------------------------------------------------------------------
vtkGraphEdge *vtkMutableDirectedGraph::AddGraphEdge(vtkIdType u, vtkIdType v)
{
  vtkEdgeType e = this->AddEdge(u, v);
  this->GraphEdge->SetSource(e.Source);
  this->GraphEdge->SetTarget(e.Target);
  this->GraphEdge->SetId(e.Id);
  return this->GraphEdge;
}

//----------------------------------------------------------------------------
vtkIdType vtkMutableDirectedGraph::AddChild(vtkIdType parent)
{
  vtkIdType v = this->AddVertex();
  this->AddEdge(parent, v);
  return v;
}

//----------------------------------------------------------------------------
void vtkMutableDirectedGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
