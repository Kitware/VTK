/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeAreaView.cxx

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

#include "vtkTreeAreaView.h"

#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCamera.h"
#include "vtkDirectedGraph.h"
#include "vtkInteractorStyle.h"
#include "vtkObjectFactory.h"
#include "vtkRenderedTreeAreaRepresentation.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkTree.h"
#include "vtkAreaLayoutStrategy.h"

vtkStandardNewMacro(vtkTreeAreaView);
//----------------------------------------------------------------------------
vtkTreeAreaView::vtkTreeAreaView()
{
  this->SetInteractionModeTo2D();
  this->ReuseSingleRepresentationOn();
}

//----------------------------------------------------------------------------
vtkTreeAreaView::~vtkTreeAreaView()
{
}

//----------------------------------------------------------------------------
vtkRenderedTreeAreaRepresentation* vtkTreeAreaView::GetTreeAreaRepresentation()
{
  vtkRenderedTreeAreaRepresentation* treeAreaRep = 0;
  for (int i = 0; i < this->GetNumberOfRepresentations(); ++i)
    {
    vtkDataRepresentation* rep = this->GetRepresentation(i);
    treeAreaRep = vtkRenderedTreeAreaRepresentation::SafeDownCast(rep);
    if (treeAreaRep)
      {
      break;
      }
    }
  if (!treeAreaRep)
    {
    vtkSmartPointer<vtkTree> g = vtkSmartPointer<vtkTree>::New();
    treeAreaRep = vtkRenderedTreeAreaRepresentation::SafeDownCast(
      this->AddRepresentationFromInput(g));
    }
  return treeAreaRep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkTreeAreaView::CreateDefaultRepresentation(
  vtkAlgorithmOutput* port)
{
  vtkRenderedTreeAreaRepresentation* rep = vtkRenderedTreeAreaRepresentation::New();
  rep->SetInputConnection(port);
  return rep;
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkTreeAreaView::SetTreeFromInputConnection(vtkAlgorithmOutput* conn)
{
  this->GetTreeAreaRepresentation()->SetInputConnection(conn);
  return this->GetTreeAreaRepresentation();
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkTreeAreaView::SetTreeFromInput(vtkTree* input)
{
  this->GetTreeAreaRepresentation()->SetInput(input);
  return this->GetTreeAreaRepresentation();
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkTreeAreaView::SetGraphFromInputConnection(vtkAlgorithmOutput* conn)
{
  this->GetTreeAreaRepresentation()->SetInputConnection(1, conn);
  return this->GetTreeAreaRepresentation();
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkTreeAreaView::SetGraphFromInput(vtkGraph* input)
{
  this->GetTreeAreaRepresentation()->SetInput(1, input);
  return this->GetTreeAreaRepresentation();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaLabelArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetLabelPriorityArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelPriorityArrayName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetLabelPriorityArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaLabelPriorityArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetAreaLabelArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelArrayName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeLabelArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetGraphEdgeLabelArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetEdgeLabelArrayName()
{
  return this->GetTreeAreaRepresentation()->GetGraphEdgeLabelArrayName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeLabelVisibility(bool vis)
{
  this->GetTreeAreaRepresentation()->SetGraphEdgeLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetEdgeLabelVisibility()
{
  return this->GetTreeAreaRepresentation()->GetGraphEdgeLabelVisibility();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaHoverArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaHoverArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetAreaHoverArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaHoverArrayName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaLabelVisibility(bool vis)
{
  this->GetTreeAreaRepresentation()->SetAreaLabelVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetAreaLabelVisibility()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelVisibility();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaColorArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetAreaColorArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaColorArrayName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetColorAreas(bool vis)
{
  this->GetTreeAreaRepresentation()->SetColorAreasByArray(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetColorAreas()
{
  return this->GetTreeAreaRepresentation()->GetColorAreasByArray();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeColorArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetGraphEdgeColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetEdgeColorArrayName()
{
  return this->GetTreeAreaRepresentation()->GetGraphEdgeColorArrayName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeColorToSplineFraction()
{
  this->GetTreeAreaRepresentation()->SetGraphEdgeColorToSplineFraction();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetColorEdges(bool vis)
{
  this->GetTreeAreaRepresentation()->SetColorGraphEdgesByArray(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetColorEdges()
{
  return this->GetTreeAreaRepresentation()->GetColorGraphEdgesByArray();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaSizeArrayName(const char* name)
{
  this->GetTreeAreaRepresentation()->SetAreaSizeArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetAreaSizeArrayName()
{
  return this->GetTreeAreaRepresentation()->GetAreaSizeArrayName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetLayoutStrategy(vtkAreaLayoutStrategy* s)
{
  this->GetTreeAreaRepresentation()->SetAreaLayoutStrategy(s);
}

//----------------------------------------------------------------------------
vtkAreaLayoutStrategy* vtkTreeAreaView::GetLayoutStrategy()
{
  return this->GetTreeAreaRepresentation()->GetAreaLayoutStrategy();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaLabelFontSize(const int size)
{
  this->GetTreeAreaRepresentation()->GetAreaLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int vtkTreeAreaView::GetAreaLabelFontSize()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeLabelFontSize(const int size)
{
  vtkTextProperty* prop = this->GetTreeAreaRepresentation()->GetGraphEdgeLabelTextProperty();
  if (prop)
    {
    prop->SetFontSize(size);
    }
}

//----------------------------------------------------------------------------
int vtkTreeAreaView::GetEdgeLabelFontSize()
{
  vtkTextProperty* prop = this->GetTreeAreaRepresentation()->GetGraphEdgeLabelTextProperty();
  if (prop)
    {
    return prop->GetFontSize();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetUseRectangularCoordinates(bool b)
{
  this->GetTreeAreaRepresentation()->SetUseRectangularCoordinates(b);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetUseRectangularCoordinates()
{
  return this->GetTreeAreaRepresentation()->GetUseRectangularCoordinates();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaToPolyData(vtkPolyDataAlgorithm* alg)
{
  this->GetTreeAreaRepresentation()->SetAreaToPolyData(alg);
}

//----------------------------------------------------------------------------
vtkPolyDataAlgorithm* vtkTreeAreaView::GetAreaToPolyData()
{
  return this->GetTreeAreaRepresentation()->GetAreaToPolyData();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaLabelMapper(vtkLabeledDataMapper* alg)
{
  this->GetTreeAreaRepresentation()->SetAreaLabelMapper(alg);
}

//----------------------------------------------------------------------------
vtkLabeledDataMapper* vtkTreeAreaView::GetAreaLabelMapper()
{
  return this->GetTreeAreaRepresentation()->GetAreaLabelMapper();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetShrinkPercentage(double p)
{
  this->GetTreeAreaRepresentation()->SetShrinkPercentage(p);
}

//----------------------------------------------------------------------------
double vtkTreeAreaView::GetShrinkPercentage()
{
  return this->GetTreeAreaRepresentation()->GetShrinkPercentage();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetBundlingStrength(double p)
{
  this->GetTreeAreaRepresentation()->SetGraphBundlingStrength(p);
}

//----------------------------------------------------------------------------
double vtkTreeAreaView::GetBundlingStrength()
{
  return this->GetTreeAreaRepresentation()->GetGraphBundlingStrength();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeScalarBarVisibility(bool b)
{
  this->GetTreeAreaRepresentation()->SetEdgeScalarBarVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetEdgeScalarBarVisibility()
{
  return this->GetTreeAreaRepresentation()->GetEdgeScalarBarVisibility();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

