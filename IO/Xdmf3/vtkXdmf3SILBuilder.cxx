/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXdmf3SILBuilder.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkXdmf3SILBuilder.h"

#include "vtkDataSetAttributes.h"
#include "vtkStringArray.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMutableDirectedGraph.h"

// As soon as num-grids (sub-grids and all) grows beyond this number, we assume
// that the grids are too numerous for the user to select individually and
// hence only the top-level grids are made accessible.
#define MAX_COLLECTABLE_NUMBER_OF_GRIDS 1000

//------------------------------------------------------------------------------
vtkXdmf3SILBuilder::vtkXdmf3SILBuilder()
{
  this->SIL = NULL;
  this->NamesArray = NULL;
  this->CrossEdgesArray = NULL;
  this->RootVertex = -1;
  this->BlocksRoot = -1;
  this->HierarchyRoot = -1;
  this->VertexCount = 0;
}

//------------------------------------------------------------------------------
vtkXdmf3SILBuilder::~vtkXdmf3SILBuilder()
{
  if (this->SIL)
  {
    this->SIL->Delete();
  }
  if (this->NamesArray)
  {
    this->NamesArray->Delete();
  }
  if (this->CrossEdgesArray)
  {
    this->CrossEdgesArray->Delete();
  }
}

//------------------------------------------------------------------------------
void vtkXdmf3SILBuilder::Initialize()
{
  if (this->SIL)
  {
    this->SIL->Delete();
  }
  this->SIL = vtkMutableDirectedGraph::New();
  this->SIL->Initialize();

  if (this->NamesArray)
  {
    this->NamesArray->Delete();
  }
  this->NamesArray = vtkStringArray::New();
  this->NamesArray->SetName("Names");
  this->SIL->GetVertexData()->AddArray(this->NamesArray);

  if (this->CrossEdgesArray)
  {
    this->CrossEdgesArray->Delete();
  }

  this->CrossEdgesArray = vtkUnsignedCharArray::New();
  this->CrossEdgesArray->SetName("CrossEdges");
  this->SIL->GetEdgeData()->AddArray(this->CrossEdgesArray);

  this->RootVertex = this->AddVertex("SIL");
  this->BlocksRoot = this->AddVertex("Blocks");
  this->HierarchyRoot = this->AddVertex("Hierarchy");
  this->AddChildEdge(RootVertex, BlocksRoot);
  this->AddChildEdge(RootVertex, HierarchyRoot);

  this->VertexCount = 0;
}

//------------------------------------------------------------------------------
vtkIdType vtkXdmf3SILBuilder::AddVertex(const char* name)
{
  this->VertexCount++;
  vtkIdType vertex = this->SIL->AddVertex();
  this->NamesArray->InsertValue(vertex, name);
  return vertex;
}

//------------------------------------------------------------------------------
vtkIdType vtkXdmf3SILBuilder::AddChildEdge(vtkIdType parent, vtkIdType child)
{
  vtkIdType id = this->SIL->AddEdge(parent, child).Id;
  this->CrossEdgesArray->InsertValue(id, 0);
  return id;
}

//------------------------------------------------------------------------------
vtkIdType vtkXdmf3SILBuilder::AddCrossEdge(vtkIdType src, vtkIdType dst)
{
  vtkIdType id = this->SIL->AddEdge(src, dst).Id;
  this->CrossEdgesArray->InsertValue(id, 1);
  return id;
}

//------------------------------------------------------------------------------
vtkIdType vtkXdmf3SILBuilder::GetRootVertex()
{
  return this->RootVertex;
}

//------------------------------------------------------------------------------
vtkIdType vtkXdmf3SILBuilder::GetBlocksRoot()
{
  return this->BlocksRoot;
}

//------------------------------------------------------------------------------
vtkIdType vtkXdmf3SILBuilder::GetHierarchyRoot()
{
  return this->HierarchyRoot;
}

//------------------------------------------------------------------------------
bool vtkXdmf3SILBuilder::IsMaxedOut()
{
  return (this->VertexCount >= MAX_COLLECTABLE_NUMBER_OF_GRIDS);
}
