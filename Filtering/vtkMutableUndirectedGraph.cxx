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
vtkCxxRevisionMacro(vtkMutableUndirectedGraph, "1.1.4.4");
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
vtkIdType vtkMutableUndirectedGraph::AddVertex(const vtkVariant& pedigreeId)
{
  vtkIdType result;
  this->AddVertexInternal(pedigreeId, &result);
  return result;
}

//----------------------------------------------------------------------------
void 
vtkMutableUndirectedGraph::AddVertex(const vtkVariant& pedigreeId, 
                                     vtkIdType *vertex)
{
  this->AddVertexInternal(pedigreeId, vertex);
}

//----------------------------------------------------------------------------
vtkIdType vtkMutableUndirectedGraph::AddVertex(vtkVariantArray *variantValueArr)
{
  return this->AddVertexInternal(variantValueArr);
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(vtkIdType u, vtkIdType v)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, &e);
  return e;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(const vtkVariant& u, vtkIdType v)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, &e);
  return e;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(vtkIdType u, const vtkVariant& v)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, &e);
  return e;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(const vtkVariant& u, const vtkVariant& v)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, &e);
  return e;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(vtkIdType u, vtkIdType v, vtkVariantArray *variantValueArr)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, &e, variantValueArr);
  return e;
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::AddEdge(vtkIdType u, vtkIdType v, vtkEdgeType *edge)
{
  this->AddEdgeInternal(u, v, false, edge);
}
//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::AddEdge(vtkIdType u, vtkIdType v, vtkEdgeType *edge, vtkVariantArray *variantValueArr)
{
  this->AddEdgeInternal(u, v, false, edge, variantValueArr);
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::AddEdge(const vtkVariant& u, vtkIdType v, vtkEdgeType *edge)
{
  this->AddEdgeInternal(u, v, false, edge);
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::AddEdge(vtkIdType u, const vtkVariant& v, vtkEdgeType *edge)
{
  this->AddEdgeInternal(u, v, false, edge);
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::AddEdge(const vtkVariant& u, const vtkVariant& v, vtkEdgeType *edge)
{
  this->AddEdgeInternal(u, v, false, edge);
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
