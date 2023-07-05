// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkHierarchicalGraphView.h"

#include "vtkDataObject.h"
#include "vtkDirectedGraph.h"
#include "vtkObjectFactory.h"
#include "vtkRenderedHierarchyRepresentation.h"
#include "vtkTree.h"
#include "vtkTrivialProducer.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHierarchicalGraphView);
//------------------------------------------------------------------------------
vtkHierarchicalGraphView::vtkHierarchicalGraphView() = default;

//------------------------------------------------------------------------------
vtkHierarchicalGraphView::~vtkHierarchicalGraphView() = default;

//------------------------------------------------------------------------------
vtkRenderedGraphRepresentation* vtkHierarchicalGraphView::GetGraphRepresentation()
{
  vtkRenderedHierarchyRepresentation* graphRep = nullptr;
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
    graphRep =
      vtkRenderedHierarchyRepresentation::SafeDownCast(this->AddRepresentationFromInput(t));
    vtkSmartPointer<vtkDirectedGraph> g = vtkSmartPointer<vtkDirectedGraph>::New();
    graphRep->SetInputData(1, g);
  }
  return graphRep;
}

//------------------------------------------------------------------------------
vtkRenderedHierarchyRepresentation* vtkHierarchicalGraphView::GetHierarchyRepresentation()
{
  return vtkRenderedHierarchyRepresentation::SafeDownCast(this->GetGraphRepresentation());
}

//------------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::CreateDefaultRepresentation(
  vtkAlgorithmOutput* port)
{
  vtkRenderedHierarchyRepresentation* rep = vtkRenderedHierarchyRepresentation::New();
  rep->SetInputConnection(port);
  return rep;
}

//------------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetHierarchyFromInputConnection(
  vtkAlgorithmOutput* conn)
{
  this->GetHierarchyRepresentation()->SetInputConnection(0, conn);
  return this->GetHierarchyRepresentation();
}

//------------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetHierarchyFromInput(vtkDataObject* input)
{
  vtkSmartPointer<vtkTrivialProducer> tp = vtkSmartPointer<vtkTrivialProducer>::New();
  tp->SetOutput(input);
  return this->SetHierarchyFromInputConnection(tp->GetOutputPort());
}

//------------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetGraphFromInputConnection(
  vtkAlgorithmOutput* conn)
{
  this->GetHierarchyRepresentation()->SetInputConnection(1, conn);
  return this->GetHierarchyRepresentation();
}

//------------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetGraphFromInput(vtkDataObject* input)
{
  vtkSmartPointer<vtkTrivialProducer> tp = vtkSmartPointer<vtkTrivialProducer>::New();
  tp->SetOutput(input);
  return this->SetGraphFromInputConnection(tp->GetOutputPort());
}

//------------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeLabelArrayName(const char* name)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeLabelArrayName(name);
}

//------------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetGraphEdgeLabelArrayName()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeLabelArrayName();
}

//------------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeLabelVisibility(bool vis)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeLabelVisibility(vis);
}

//------------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetGraphEdgeLabelVisibility()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeLabelVisibility();
}

//------------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeColorArrayName(const char* name)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeColorArrayName(name);
}

//------------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetGraphEdgeColorArrayName()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeColorArrayName();
}

//------------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeColorToSplineFraction()
{
  this->GetHierarchyRepresentation()->SetGraphEdgeColorToSplineFraction();
}

//------------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetColorGraphEdgesByArray(bool vis)
{
  this->GetHierarchyRepresentation()->SetColorGraphEdgesByArray(vis);
}

//------------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetColorGraphEdgesByArray()
{
  return this->GetHierarchyRepresentation()->GetColorGraphEdgesByArray();
}

//------------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphVisibility(bool vis)
{
  this->GetHierarchyRepresentation()->SetGraphVisibility(vis);
}

//------------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetGraphVisibility()
{
  return this->GetHierarchyRepresentation()->GetGraphVisibility();
}

//------------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetBundlingStrength(double strength)
{
  this->GetHierarchyRepresentation()->SetBundlingStrength(strength);
}

//------------------------------------------------------------------------------
double vtkHierarchicalGraphView::GetBundlingStrength()
{
  return this->GetHierarchyRepresentation()->GetBundlingStrength();
}

//------------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetGraphEdgeLabelFontSize(int size)
{
  this->GetHierarchyRepresentation()->SetGraphEdgeLabelFontSize(size);
}

//------------------------------------------------------------------------------
int vtkHierarchicalGraphView::GetGraphEdgeLabelFontSize()
{
  return this->GetHierarchyRepresentation()->GetGraphEdgeLabelFontSize();
}

//------------------------------------------------------------------------------
void vtkHierarchicalGraphView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
