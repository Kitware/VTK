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
vtkCxxRevisionMacro(vtkMutableUndirectedGraph, "1.1.4.5");
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
vtkIdType vtkMutableUndirectedGraph::AddVertex(vtkVariantArray *propertyArr)
{
  vtkIdType vertex;
  this->AddVertexInternal(propertyArr, &vertex);
  return vertex;
}

//----------------------------------------------------------------------------
vtkIdType vtkMutableUndirectedGraph::AddVertex(const vtkVariant& pedigreeId)
{
  vtkIdType vertex;
  this->AddVertexInternal(pedigreeId, &vertex);
  return vertex;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(vtkIdType u, vtkIdType v,
                                               vtkVariantArray *propertyArr)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, propertyArr, &e);
  return e;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(const vtkVariant& u, vtkIdType v,
                                               vtkVariantArray *propertyArr)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, propertyArr, &e);
  return e;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(vtkIdType u, const vtkVariant& v,
                                               vtkVariantArray *propertyArr)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, propertyArr, &e);
  return e;
}

//----------------------------------------------------------------------------
vtkEdgeType vtkMutableUndirectedGraph::AddEdge(const vtkVariant& u, 
                                               const vtkVariant& v,
                                               vtkVariantArray *propertyArr)
{
  vtkEdgeType e;
  this->AddEdgeInternal(u, v, false, propertyArr, &e);
  return e;
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddVertex(vtkVariantArray *propertyArr)
{
  this->AddVertexInternal(propertyArr, 0);
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddVertex(const vtkVariant& pedigreeId)
{
  this->AddVertexInternal(pedigreeId, 0);
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddEdge(vtkIdType u, vtkIdType v,
                                            vtkVariantArray *propertyArr)
{
  this->AddEdgeInternal(u, v, false, propertyArr, 0);
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddEdge(const vtkVariant& u, vtkIdType v,
                                            vtkVariantArray *propertyArr)
{
  this->AddEdgeInternal(u, v, false, propertyArr, 0);
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddEdge(vtkIdType u, const vtkVariant& v,
                                            vtkVariantArray *propertyArr)
{
  this->AddEdgeInternal(u, v, false, propertyArr, 0);
}

//----------------------------------------------------------------------------
void vtkMutableUndirectedGraph::LazyAddEdge(const vtkVariant& u, 
                                            const vtkVariant& v,
                                            vtkVariantArray *propertyArr)
{
  this->AddEdgeInternal(u, v, false, propertyArr, 0);
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
