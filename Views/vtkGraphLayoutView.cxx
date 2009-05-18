/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutView.cxx

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

#include "vtkGraphLayoutView.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCamera.h"
#include "vtkDirectedGraph.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkInteractorStyle.h"
#include "vtkObjectFactory.h"
#include "vtkRenderedGraphRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkTextProperty.h"

vtkCxxRevisionMacro(vtkGraphLayoutView, "1.63");
vtkStandardNewMacro(vtkGraphLayoutView);
//----------------------------------------------------------------------------
vtkGraphLayoutView::vtkGraphLayoutView()
{
  this->SetInteractionModeTo2D();
  this->SetLabelPlacementModeToDynamic2D();
  this->ReuseSingleRepresentationOn();
}

//----------------------------------------------------------------------------
vtkGraphLayoutView::~vtkGraphLayoutView()
{
}

//----------------------------------------------------------------------------
vtkRenderedGraphRepresentation* vtkGraphLayoutView::GetGraphRepresentation()
{
  vtkRenderedGraphRepresentation* graphRep = 0;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkDataRepresentation* rep = this->GetRepresentation(i);
    graphRep = vtkRenderedGraphRepresentation::SafeDownCast(rep);
    if (graphRep)
      {
      break;
      }
    }
  if (!graphRep)
    {
    vtkSmartPointer<vtkDirectedGraph> g = vtkSmartPointer<vtkDirectedGraph>::New();
    graphRep = vtkRenderedGraphRepresentation::SafeDownCast(
      this->AddRepresentationFromInput(g));
    }
  return graphRep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkGraphLayoutView::CreateDefaultRepresentation(
  vtkAlgorithmOutput* port)
{
  vtkRenderedGraphRepresentation* rep = vtkRenderedGraphRepresentation::New();
  rep->SetInputConnection(port);
  return rep;
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexLabelArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetVertexLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetVertexLabelArrayName()
{
  return this->GetGraphRepresentation()->GetVertexLabelArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLabelArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEdgeLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEdgeLabelArrayName()
{
  return this->GetGraphRepresentation()->GetEdgeLabelArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexLabelVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetVertexLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetVertexLabelVisibility()
{
  return this->GetGraphRepresentation()->GetVertexLabelVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetEdgeVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetEdgeVisibility()
{
  return this->GetGraphRepresentation()->GetEdgeVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLabelVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetEdgeLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetEdgeLabelVisibility()
{
  return this->GetGraphRepresentation()->GetEdgeLabelVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexColorArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetVertexColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetVertexColorArrayName()
{
  return this->GetGraphRepresentation()->GetVertexColorArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetColorVertices(bool vis)
{
  this->GetGraphRepresentation()->SetColorVerticesByArray(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetColorVertices()
{
  return this->GetGraphRepresentation()->GetColorVerticesByArray();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexScalarBarVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetVertexScalarBarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetVertexScalarBarVisibility()
{
  return this->GetGraphRepresentation()->GetVertexScalarBarVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeColorArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEdgeColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEdgeColorArrayName()
{
  return this->GetGraphRepresentation()->GetEdgeColorArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetColorEdges(bool vis)
{
  this->GetGraphRepresentation()->SetColorEdgesByArray(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetColorEdges()
{
  return this->GetGraphRepresentation()->GetColorEdgesByArray();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeScalarBarVisibility(bool vis)
{
  this->GetGraphRepresentation()->SetEdgeScalarBarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetEdgeScalarBarVisibility()
{
  return this->GetGraphRepresentation()->GetEdgeScalarBarVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEnabledEdgesArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEnabledEdgesArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEnabledEdgesArrayName()
{
  return this->GetGraphRepresentation()->GetEnabledEdgesArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEnableEdgesByArray(bool vis)
{
  this->GetGraphRepresentation()->SetEnableEdgesByArray(vis);
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::GetEnableEdgesByArray()
{
  return this->GetGraphRepresentation()->GetEnableEdgesByArray();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEnabledVerticesArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetEnabledVerticesArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEnabledVerticesArrayName()
{
  return this->GetGraphRepresentation()->GetEnabledVerticesArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEnableVerticesByArray(bool vis)
{
  this->GetGraphRepresentation()->SetEnableVerticesByArray(vis);
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::GetEnableVerticesByArray()
{
  return this->GetGraphRepresentation()->GetEnableVerticesByArray();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetScaledGlyphs(bool arg)
{
  this->GetGraphRepresentation()->SetScaling(arg);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetScaledGlyphs()
{
  return this->GetGraphRepresentation()->GetScaling();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetScalingArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetScalingArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetScalingArrayName()
{
  return this->GetGraphRepresentation()->GetScalingArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetIconArrayName(const char* name)
{
  this->GetGraphRepresentation()->SetVertexIconArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetIconArrayName()
{
  return this->GetGraphRepresentation()->GetVertexIconArrayName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::AddIconType(char *type, int index)
{
  this->GetGraphRepresentation()->AddVertexIconType(type, index);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::ClearIconTypes()
{
  this->GetGraphRepresentation()->ClearVertexIconTypes();
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::IsLayoutComplete()
{
  return this->GetGraphRepresentation()->IsLayoutComplete();
}
  
//----------------------------------------------------------------------------
void vtkGraphLayoutView::UpdateLayout()
{
  this->GetGraphRepresentation()->UpdateLayout();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetLayoutStrategy(vtkGraphLayoutStrategy* s)
{
  this->GetGraphRepresentation()->SetLayoutStrategy(s);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetLayoutStrategy(const char* name)
{
  this->GetGraphRepresentation()->SetLayoutStrategy(name);
}

//----------------------------------------------------------------------------
vtkGraphLayoutStrategy* vtkGraphLayoutView::GetLayoutStrategy()
{
  return this->GetGraphRepresentation()->GetLayoutStrategy();
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetLayoutStrategyName()
{
  return this->GetGraphRepresentation()->GetLayoutStrategyName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLayoutStrategy(vtkEdgeLayoutStrategy *s)
{
  this->GetGraphRepresentation()->SetEdgeLayoutStrategy(s);
}

//----------------------------------------------------------------------------
vtkEdgeLayoutStrategy* vtkGraphLayoutView::GetEdgeLayoutStrategy()
{
  return this->GetGraphRepresentation()->GetEdgeLayoutStrategy();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLayoutStrategy(const char* name)
{
  this->GetGraphRepresentation()->SetEdgeLayoutStrategy(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEdgeLayoutStrategyName()
{
  return this->GetGraphRepresentation()->GetEdgeLayoutStrategyName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetIconAlignment(int alignment)
{
  this->GetGraphRepresentation()->SetVertexIconAlignment(alignment);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetIconVisibility(bool b)
{
  this->GetGraphRepresentation()->SetVertexIconVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetIconVisibility()
{
  return this->GetGraphRepresentation()->GetVertexIconVisibility();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexLabelFontSize(const int size)
{
  this->GetGraphRepresentation()->GetVertexLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::GetVertexLabelFontSize()
{
  return this->GetGraphRepresentation()->GetVertexLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLabelFontSize(const int size)
{
  this->GetGraphRepresentation()->GetEdgeLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::GetEdgeLabelFontSize()
{
  return this->GetGraphRepresentation()->GetEdgeLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::ZoomToSelection()
{
  double bounds[6];
  this->GetGraphRepresentation()->ComputeSelectedGraphBounds(bounds);
  this->Renderer->ResetCamera(bounds);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

