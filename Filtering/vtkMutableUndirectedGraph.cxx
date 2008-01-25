/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMutableUndirectedGraph.cxx

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

#include "vtkMutableUndirectedGraph.h"

#include "vtkGraphEdge.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
// class vtkMutableUndirectedGraph
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkMutableUndirectedGraph, "1.1");
vtkStandardNewMacro(vtkMutableUndirectedGraph);
//----------------------------------------------------------------------------
vtkMutableUndirectedGraph::vtkMutableUndirectedGraph()
{
  this->GraphEdge = vtkGraphEdge::New();
}

//----------------------------------------------------------------------------
vtkMutableUndirectedGraph::~vtkMutableUndirectedGraph()
{
  this->GraphEdge->Delete();
}

//----------------------------------------------------------------------------
vtkIdType vtkMutableUndirectedGraph::AddVertex()
{
  return this->AddVertexInternal();
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(vtkIdType u, vtkIdType v)
{
  return this->AddEdgeInternal(u, v, false);
}

//----------------------------------------------------------------------------
vtkGraphEdge *vtkMutableUndirectedGraph::AddGraphEdge(vtkIdType u, vtkIdType v)
{
  vtkEdgeType e = this->AddEdge(u, v);
  this->GraphEdge->SetSource(e.Source);
  this->GraphEdge->SetTarget(e.Target);
  this->GraphEdge->SetId(e.Id);
  return this->GraphEdge;
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
