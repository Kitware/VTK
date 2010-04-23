/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalGraphView.cxx
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHierarchicalGraphView.h"

#include "vtkDataObject.h"
#include "vtkDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkRenderedHierarchyRepresentation.h"
#include "vtkTree.h"

vtkStandardNewMacro(vtkHierarchicalGraphView);
//----------------------------------------------------------------------------
vtkHierarchicalGraphView::vtkHierarchicalGraphView()
{
}

//----------------------------------------------------------------------------
vtkHierarchicalGraphView::~vtkHierarchicalGraphView()
{  
}

//----------------------------------------------------------------------------
vtkRenderedGraphRepresentation* vtkHierarchicalGraphView::GetGraphRepresentation()
{
  vtkRenderedHierarchyRepresentation* graphRep = 0;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkDataRepresentation* rep = this->GetRepresentation(i);
    graphRep = vtkRenderedHierarchyRepresentation::SafeDownCast(rep);
    if (graphRep)
      {
      break;
      }
    }
  if (!graphRep)
    {
    vtkSmartPointer<vtkTree> t = vtkSmartPointer<vtkTree>::New();
    graphRep = vtkRenderedHierarchyRepresentation::SafeDownCast(
      this->AddRepresentationFromInput(t));
    vtkSmartPointer<vtkDirectedGraph> g = vtkSmartPointer<vtkDirectedGraph>::New();
    graphRep->SetInput(1, g);
    }
  return graphRep;
}

//----------------------------------------------------------------------------
vtkRenderedHierarchyRepresentation* vtkHierarchicalGraphView::GetHierarchyRepresentation()
{
  return vtkRenderedHierarchyRepresentation::SafeDownCast(this->GetGraphRepresentation());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::CreateDefaultRepresentation(
  vtkAlgorithmOutput* port)
{
  vtkRenderedHierarchyRepresentation* rep = vtkRenderedHierarchyRepresentation::New();
  rep->SetInputConnection(port);
  return rep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetHierarchyFromInputConnection(vtkAlgorithmOutput* conn)
{
  this->GetHierarchyRepresentation()->SetInputConnection(0, conn);
  return this->GetHierarchyRepresentation();
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetHierarchyFromInput(vtkDataObject* input)
{
  return this->SetHierarchyFromInputConnection(input->GetProducerPort());
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetGraphFromInputConnection(vtkAlgorithmOutput* conn)
{
  this->GetHierarchyRepresentation()->SetInputConnection(1, conn);
  return this->GetHierarchyRepresentation();
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetGraphFromInput(vtkDataObject* input)
{
  return this->SetGraphFromInputConnection(input->GetProducerPort());
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeLabelArrayName(const char* name)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetGraphEdgeLabelArrayName()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeLabelArrayName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeLabelVisibility(bool vis)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetGraphEdgeLabelVisibility()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeLabelVisibility();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeColorArrayName(const char* name)
{   
  this->GetHierarchyRepresentation()->SetGraphEdgeColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetGraphEdgeColorArrayName()
{   
  return this->GetHierarchyRepresentation()->GetGraphEdgeColorArrayName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeColorToSplineFraction()
{
  this->GetHierarchyRepresentation()->SetGraphEdgeColorToSplineFraction();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetColorGraphEdgesByArray(bool vis)
{
  this->GetHierarchyRepresentation()->SetColorGraphEdgesByArray(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetColorGraphEdgesByArray()
{
  return this->GetHierarchyRepresentation()->GetColorGraphEdgesByArray();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphVisibility(bool vis)
{
  this->GetHierarchyRepresentation()->SetGraphVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetGraphVisibility()
{
  return this->GetHierarchyRepresentation()->GetGraphVisibility();
}

// ----------------------------------------------------------------------
void vtkHierarchicalGraphView::SetBundlingStrength(double strength)
{
  this->GetHierarchyRepresentation()->SetBundlingStrength(strength);
}

// ----------------------------------------------------------------------
double vtkHierarchicalGraphView::GetBundlingStrength()
{
  return this->GetHierarchyRepresentation()->GetBundlingStrength();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeLabelFontSize(const int size)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeLabelFontSize(size);
}

//----------------------------------------------------------------------------
int vtkHierarchicalGraphView::GetGraphEdgeLabelFontSize()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeLabelFontSize();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

