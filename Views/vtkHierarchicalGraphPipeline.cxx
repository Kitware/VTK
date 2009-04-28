/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalGraphPipeline.cxx

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

#include "vtkHierarchicalGraphPipeline.h"

#include "vtkActor.h"
#include "vtkApplyColors.h"
#include "vtkConvertSelection.h"
#include "vtkGraphHierarchicalBundleEdges.h"
#include "vtkGraphToPolyData.h"
#include "vtkInformation.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSplineFilter.h"
#include "vtkSplineGraphEdges.h"
#include "vtkTextProperty.h"
#include "vtkView.h"
#include "vtkViewTheme.h"

vtkCxxRevisionMacro(vtkHierarchicalGraphPipeline, "1.2");
vtkStandardNewMacro(vtkHierarchicalGraphPipeline);

vtkHierarchicalGraphPipeline::vtkHierarchicalGraphPipeline()
{
  this->ApplyColors = vtkApplyColors::New();
  this->Bundle = vtkGraphHierarchicalBundleEdges::New();
  this->GraphToPoly = vtkGraphToPolyData::New();
  this->Spline = vtkSplineGraphEdges::New();
  this->Mapper = vtkPolyDataMapper::New();
  this->Actor = vtkActor::New();
  this->TextProperty = vtkTextProperty::New();

  this->ColorArrayNameInternal = 0;

  /*
  <graphviz>
  digraph {
    "Graph input" -> Bundle
    "Tree input" -> Bundle
    Bundle -> Spline -> ApplyColors -> GraphToPoly -> Mapper -> Actor
  }
  </graphviz>
  */

  this->Spline->SetInputConnection(this->Bundle->GetOutputPort());
  this->ApplyColors->SetInputConnection(this->Spline->GetOutputPort());
  this->GraphToPoly->SetInputConnection(this->ApplyColors->GetOutputPort());
  this->Mapper->SetInputConnection(this->GraphToPoly->GetOutputPort());
  this->Actor->SetMapper(this->Mapper);

  this->ApplyColors->SetUseCellLookupTable(true);
  this->Mapper->SetScalarModeToUseCellFieldData();
  this->Mapper->SelectColorArray("vtkApplyColors color");
  this->Mapper->ScalarVisibilityOn();
  this->Actor->PickableOn();

  // Make sure this gets rendered on top
  this->Actor->SetPosition(0.0, 0.0, 1.0);

  this->Bundle->SetBundlingStrength(0.5);
  //this->Spline->GetSplineFilter()->SetMaximumNumberOfSubdivisions(16);
}

vtkHierarchicalGraphPipeline::~vtkHierarchicalGraphPipeline()
{
  this->SetColorArrayNameInternal(0);
  this->ApplyColors->Delete();
  this->Bundle->Delete();
  this->GraphToPoly->Delete();
  this->Spline->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->TextProperty->Delete();
}

void vtkHierarchicalGraphPipeline::SetBundlingStrength(double strength)
{
  this->Bundle->SetBundlingStrength(strength);
}

double vtkHierarchicalGraphPipeline::GetBundlingStrength()
{
  return this->Bundle->GetBundlingStrength();
}

// TODO: edge labeling
void vtkHierarchicalGraphPipeline::SetLabelArrayName(const char* name)
{
  (void)name;
}

const char* vtkHierarchicalGraphPipeline::GetLabelArrayName()
{
  return 0;
}

void vtkHierarchicalGraphPipeline::SetLabelVisibility(bool vis)
{
  (void)vis;
}

bool vtkHierarchicalGraphPipeline::GetLabelVisibility()
{
  return false;
}

void vtkHierarchicalGraphPipeline::SetLabelTextProperty(vtkTextProperty* prop)
{
  this->TextProperty->ShallowCopy(prop);
}

vtkTextProperty* vtkHierarchicalGraphPipeline::GetLabelTextProperty()
{
  return this->TextProperty;
}

void vtkHierarchicalGraphPipeline::SetColorArrayName(const char* name)
{
  this->SetColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(1, 0, 0,
    vtkDataObject::FIELD_ASSOCIATION_EDGES, name);
}

const char* vtkHierarchicalGraphPipeline::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

void vtkHierarchicalGraphPipeline::SetColorEdgesByArray(bool vis)
{
  this->ApplyColors->SetUseCellLookupTable(vis);
}

bool vtkHierarchicalGraphPipeline::GetColorEdgesByArray()
{
  return this->ApplyColors->GetUseCellLookupTable();
}

void vtkHierarchicalGraphPipeline::SetVisibility(bool vis)
{
  this->Actor->SetVisibility(vis);
}

bool vtkHierarchicalGraphPipeline::GetVisibility()
{
  return this->Actor->GetVisibility() ? true : false;
}

void vtkHierarchicalGraphPipeline::SetupInputConnections(
  vtkAlgorithmOutput* graphConn,
  vtkAlgorithmOutput* treeConn,
  vtkAlgorithmOutput* annConn,
  vtkAlgorithmOutput* selConn)
{
  this->Bundle->SetInputConnection(0, graphConn);
  this->Bundle->SetInputConnection(1, treeConn);
  this->ApplyColors->SetInputConnection(1, annConn);
  this->ApplyColors->SetInputConnection(2, selConn);
}

vtkSelection* vtkHierarchicalGraphPipeline::ConvertSelection(vtkView* view, vtkSelection* sel)
{
  vtkSelection* converted = vtkSelection::New();
  for (unsigned int j = 0; j < sel->GetNumberOfNodes(); ++j)
    {
    vtkSelectionNode* node = sel->GetNode(j);
    vtkProp* prop = vtkProp::SafeDownCast(
      node->GetProperties()->Get(vtkSelectionNode::PROP()));
    if (prop == this->Actor)
      {
      vtkDataObject* input = this->Bundle->GetInputDataObject(0, 0);
      vtkDataObject* poly = this->GraphToPoly->GetOutput();
      vtkSmartPointer<vtkSelection> edgeSel =
        vtkSmartPointer<vtkSelection>::New();
      vtkSmartPointer<vtkSelectionNode> nodeCopy =
        vtkSmartPointer<vtkSelectionNode>::New();
      nodeCopy->ShallowCopy(node);
      nodeCopy->GetProperties()->Remove(vtkSelectionNode::PROP());
      edgeSel->AddNode(nodeCopy);
      vtkSelection* polyConverted = vtkConvertSelection::ToSelectionType(
        edgeSel, poly, vtkSelectionNode::PEDIGREEIDS);
      for (unsigned int i = 0; i < polyConverted->GetNumberOfNodes(); ++i)
        {
        polyConverted->GetNode(i)->SetFieldType(vtkSelectionNode::EDGE);
        }
      vtkSelection* edgeConverted = vtkConvertSelection::ToSelectionType(
        polyConverted, input, view->GetSelectionType());
      for (unsigned int i = 0; i < edgeConverted->GetNumberOfNodes(); ++i)
        {
        converted->AddNode(edgeConverted->GetNode(i));
        }
      polyConverted->Delete();
      edgeConverted->Delete();
      }
    }
  return converted;
}

void vtkHierarchicalGraphPipeline::ApplyViewTheme(vtkViewTheme* theme)
{
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());
  vtkScalarsToColors* oldLut = this->ApplyColors->GetCellLookupTable();
  if (!theme->LookupMatchesCellTheme(oldLut))
    {
    vtkSmartPointer<vtkLookupTable> lut =
      vtkSmartPointer<vtkLookupTable>::New();
    lut->SetHueRange(theme->GetCellHueRange());
    lut->SetSaturationRange(theme->GetCellSaturationRange());
    lut->SetValueRange(theme->GetCellValueRange());
    lut->SetAlphaRange(theme->GetCellAlphaRange());
    lut->Build();
    this->ApplyColors->SetCellLookupTable(lut);
    }
}

void vtkHierarchicalGraphPipeline::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Actor: ";
  if (this->Actor && this->Bundle->GetNumberOfInputConnections(0) > 0)
    {
    os << "\n";
    this->Actor->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)\n";
    }
}
