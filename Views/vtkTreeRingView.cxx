/*=========================================================================
  
Program:   Visualization Toolkit
Module:    vtkTreeRingView.cxx

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

#include "vtkTreeRingView.h"

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleTreeRingHover.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkTreeFieldAggregator.h"
#include "vtkTreeLevelsFilter.h"
#include "vtkTreeRingLayout.h"
#include "vtkTreeRingDefaultLayoutStrategy.h"
#include "vtkTreeRingReversedLayoutStrategy.h"
#include "vtkTreeRingToPolyData.h"
#include "vtkViewTheme.h"

vtkCxxRevisionMacro(vtkTreeRingView, "1.6");
vtkStandardNewMacro(vtkTreeRingView);
//----------------------------------------------------------------------------
vtkTreeRingView::vtkTreeRingView()
{
  this->ColorArrayNameInternal = 0;
  
  this->TreeLevelsFilter      = vtkTreeLevelsFilter::New();
  this->TreeFieldAggregator   = vtkTreeFieldAggregator::New();
  this->TreeRingLayout        = vtkTreeRingLayout::New();
  this->TreeRingDefaultLayout = vtkTreeRingDefaultLayoutStrategy::New();
  this->TreeRingReversedLayout= vtkTreeRingReversedLayoutStrategy::New();
  this->TreeRingToPolyData    = vtkTreeRingToPolyData::New();
  this->TreeRingMapper        = vtkPolyDataMapper::New();
  this->TreeRingActor         = vtkActor::New();
  this->LabelMapper           = vtkDynamic2DLabelMapper::New();
  this->LabelActor            = vtkActor2D::New();
  this->ColorLUT              = vtkLookupTable::New();
  
  // Replace the interactor style
  vtkInteractorStyleTreeRingHover* style = vtkInteractorStyleTreeRingHover::New();
  this->SetInteractorStyle(style);
  style->SetLayout(this->TreeRingLayout);
  style->AddObserver(vtkCommand::UserEvent, this->GetObserver());
  style->Delete();
  
  // Set up view
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  
  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();
  
  // Set up representation
  this->TreeFieldAggregator->SetLeafVertexUnitSize(false);
  this->TreeFieldAggregator->SetMinValue(1e-10);
  this->TreeFieldAggregator->SetLogScale(false);
  this->ColorLUT->SetHueRange(0.667, 0);
  this->ColorLUT->Build();
  this->TreeRingMapper->SetLookupTable(ColorLUT);
  this->LabelMapper->SetLabelModeToLabelFieldData();
  this->LabelMapper->GetLabelTextProperty()->SetColor(1,1,1);
  this->LabelMapper->GetLabelTextProperty()->SetJustificationToCentered();
  this->LabelMapper->GetLabelTextProperty()->SetVerticalJustificationToCentered();
  this->LabelMapper->GetLabelTextProperty()->SetFontSize(12);
  this->LabelMapper->GetLabelTextProperty()->SetItalic(0);
  this->LabelMapper->GetLabelTextProperty()->SetLineOffset(0);
  this->LabelMapper->SetPriorityArrayName("leaf_count");
  this->LabelActor->PickableOff();
  
  // Set default properties
  this->SetSizeArrayName("size");
  this->SetHoverArrayName("name");
  this->SetLabelArrayName("name");
//  this->SetFontSizeRange(24, 10);
  this->SetLayoutStrategyToDefault();
  
  // Wire pipeline
  this->TreeFieldAggregator->SetInputConnection(
    this->TreeLevelsFilter->GetOutputPort());
  this->TreeRingLayout->SetInputConnection(
    this->TreeFieldAggregator->GetOutputPort());
  this->TreeRingToPolyData->SetInputConnection(
    this->TreeRingLayout->GetOutputPort());
  this->TreeRingMapper->SetInputConnection(
    this->TreeRingToPolyData->GetOutputPort());
  this->TreeRingActor->SetMapper(this->TreeRingMapper);
  this->LabelMapper->SetInputConnection(
    this->TreeRingLayout->GetOutputPort());
  this->LabelActor->SetMapper(this->LabelMapper);
}

//----------------------------------------------------------------------------
vtkTreeRingView::~vtkTreeRingView()
{
  this->SetColorArrayNameInternal(0);
  
  // Delete objects
  this->TreeLevelsFilter->Delete();
  this->TreeFieldAggregator->Delete();
  this->TreeRingLayout->Delete();
  this->TreeRingDefaultLayout->Delete();
  this->TreeRingReversedLayout->Delete();
  this->TreeRingToPolyData->Delete();
  this->TreeRingMapper->Delete();
  this->TreeRingActor->Delete();
  this->LabelMapper->Delete();
  this->LabelActor->Delete();
  this->ColorLUT->Delete();
}

//----------------------------------------------------------------------------
void vtkTreeRingView::SetSizeArrayName(const char* name)
{
  this->TreeFieldAggregator->SetField(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeRingView::GetSizeArrayName()
{
  return this->TreeFieldAggregator->GetField();
}

//----------------------------------------------------------------------------
void vtkTreeRingView::SetHoverArrayName(const char* name)
{
  vtkInteractorStyleTreeRingHover::SafeDownCast(this->InteractorStyle)->SetLabelField(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeRingView::GetHoverArrayName()
{
  return vtkInteractorStyleTreeRingHover::SafeDownCast(this->InteractorStyle)->GetLabelField();
}

//----------------------------------------------------------------------------
void vtkTreeRingView::SetLabelArrayName(const char* name)
{
  this->LabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeRingView::GetLabelArrayName()
{
  return this->LabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkTreeRingView::SetColorArrayName(const char *field)
{
  this->TreeRingMapper->SetScalarModeToUseCellFieldData();
  this->TreeRingMapper->SelectColorArray(field);
  this->SetColorArrayNameInternal(field);
}

//----------------------------------------------------------------------------
const char* vtkTreeRingView::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkTreeRingView::SetLayoutStrategyToDefault()
{
  this->SetLayoutStrategy("Default");
}

//----------------------------------------------------------------------------
void vtkTreeRingView::SetLayoutStrategyToReversed()
{
  this->SetLayoutStrategy("Reversed");
}

//----------------------------------------------------------------------------
void vtkTreeRingView::SetLayoutStrategy(const char* name)
{
  if (!strcmp(name, "Default"))
    {
    this->TreeRingLayout->SetLayoutStrategy(this->TreeRingDefaultLayout);
    }
  else if (!strcmp(name, "Reversed"))
    {
    this->TreeRingLayout->SetLayoutStrategy(this->TreeRingReversedLayout);
    }
  else
    {
    vtkErrorMacro("Unknown layout name: " << name);
    }
}

//----------------------------------------------------------------------------
void vtkTreeRingView::SetupRenderWindow(vtkRenderWindow* win)
{
  this->Superclass::SetupRenderWindow(win);
  win->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
}

//----------------------------------------------------------------------------
void vtkTreeRingView::AddInputConnection( int port, int item,
                                          vtkAlgorithmOutput* conn,
                                          vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  if( port != 0 || item != 0 )
    {
    vtkErrorMacro("This view only supports one representation.");
    }
  else if (this->TreeLevelsFilter->GetNumberOfInputConnections(0) == 0)
    {
    this->TreeLevelsFilter->SetInputConnection(conn);
    
    this->Renderer->AddActor(this->TreeRingActor);
    this->Renderer->AddActor(this->LabelActor);
    this->Renderer->ResetCamera();
    }
  else
    {
    vtkErrorMacro("This view only supports one representation.");
    }
}

//----------------------------------------------------------------------------
void vtkTreeRingView::RemoveInputConnection( int port, int item,
                                             vtkAlgorithmOutput* conn,
                                             vtkAlgorithmOutput* vtkNotUsed(selectionConn))
{
  if( port != 0 || item != 0 )
    {
    vtkErrorMacro("This view only supports one representation.");
    }
  
  if (this->TreeLevelsFilter->GetNumberOfInputConnections(0) > 0 &&
      this->TreeLevelsFilter->GetInputConnection(0, 0) == conn)
    {
    this->TreeLevelsFilter->RemoveInputConnection(0, conn);
    
    this->Renderer->RemoveActor(this->TreeRingActor);
    this->Renderer->RemoveActor(this->LabelActor);
    }  
}

//----------------------------------------------------------------------------
void vtkTreeRingView::ProcessEvents(
  vtkObject* caller, unsigned long eventId, void* callData)
{
  if (caller == this->InteractorStyle && eventId == vtkCommand::UserEvent)
    {
    // Create the selection
    vtkSmartPointer<vtkSelection> selection = vtkSmartPointer<vtkSelection>::New();
    vtkSmartPointer<vtkSelectionNode> node = vtkSmartPointer<vtkSelectionNode>::New();
    vtkSmartPointer<vtkIdTypeArray> list = vtkSmartPointer<vtkIdTypeArray>::New();
    vtkIdType* id = reinterpret_cast<vtkIdType*>(callData);
    if (*id >= 0)
      {
      list->InsertNextValue(*id);
      }
    node->SetSelectionList(list);
    // TODO: This should really be pedigree ids.
    node->SetContentType(vtkSelectionNode::INDICES);
    selection->AddNode(node);
    
    // Call select on the representation(s)
    this->GetRepresentation()->Select(this, selection);
    }
  else
    {
    Superclass::ProcessEvents(caller, eventId, callData);
    }
}

//----------------------------------------------------------------------------
void vtkTreeRingView::PrepareForRendering()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    return;
    }
  
  // Make sure the input connection is up to date.
  vtkAlgorithmOutput* conn = rep->GetInputConnection();
  if (this->TreeLevelsFilter->GetInputConnection(0, 0) != conn)
    {
    this->RemoveInputConnection(0, 0, this->TreeLevelsFilter->GetInputConnection(0, 0), 0);
    this->AddInputConnection(0, 0, conn, rep->GetSelectionConnection());
    }
  
  // Use the most recent selection
  vtkAlgorithm* alg = rep->GetSelectionConnection()->GetProducer();
  alg->Update();
  vtkSelection* selection = vtkSelection::SafeDownCast(
    alg->GetOutputDataObject(rep->GetSelectionConnection()->GetIndex()));
  // TODO: Should be pedigree ids.
  vtkSelectionNode* node = selection->GetNode(0);
  if (!node)
    {
    vtkErrorMacro("Selection should have a single node.");
    return;
    }
  if (node->GetContentType() != vtkSelectionNode::INDICES)
    {
    vtkErrorMacro("Can only handle INDICES selections.");
    return;
    }
  vtkIdTypeArray* arr = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
  vtkIdType id = -1;
  if (arr && arr->GetNumberOfTuples() > 0)
    {
    id = arr->GetValue(0);
    }
  vtkInteractorStyleTreeRingHover::SafeDownCast(this->InteractorStyle)->HighLightItem(id);
  
  // Update the pipeline up until the treering to polydata
  this->TreeRingToPolyData->Update();
  
  // Try to find the range the user-specified color array.
  // If we cannot find that array, use the scalar range. 
  double range[2];
  vtkDataArray* array = 0; 
  if (this->GetColorArrayName())
    {
    array = this->TreeRingToPolyData->GetOutput()->
      GetCellData()->GetArray(this->GetColorArrayName());
    }
  if (array)
    {
    array->GetRange(range);    
    }
  else
    {
    this->TreeRingToPolyData->GetOutput()->GetScalarRange(range);
    }
  this->TreeRingMapper->SetScalarRange(range[0], range[1]);
  
  this->Superclass::PrepareForRendering();
}

//----------------------------------------------------------------------------
void vtkTreeRingView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  
  double color[3];
  theme->GetSelectedPointColor(color);
  vtkInteractorStyleTreeRingHover::SafeDownCast(this->InteractorStyle)->
    SetSelectionLightColor(color[0], color[1], color[2]);
}

void vtkTreeRingView::SetSectorShrinkPercentage( double shrinkFactor )
{
  this->TreeRingToPolyData->SetShrinkPercentage( shrinkFactor );
}

double vtkTreeRingView::GetSectorShrinkPercentage()
{
  return this->TreeRingToPolyData->GetShrinkPercentage();
}

void vtkTreeRingView::SetRootAngles( double start, double end )
{
  this->TreeRingDefaultLayout->SetRootStartAngle( start );
  this->TreeRingDefaultLayout->SetRootEndAngle( end );
  this->TreeRingReversedLayout->SetRootStartAngle( start );
  this->TreeRingReversedLayout->SetRootEndAngle( end );
}

//----------------------------------------------------------------------------
void vtkTreeRingView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TreeLevelsFilter: " << endl;
  this->TreeLevelsFilter->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeFieldAggregator: " << endl;
  this->TreeFieldAggregator->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeRingLayout: " << endl;
  this->TreeRingLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeRingDefaultLayout: " << endl;
  this->TreeRingDefaultLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeRingReversedLayout: " << endl;
  this->TreeRingReversedLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeRingToPolyData: " << endl;
  this->TreeRingToPolyData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeRingMapper: " << endl;
  this->TreeRingMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LabelMapper: " << endl;
  this->LabelMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ColorLUT: " << endl;
  this->ColorLUT->PrintSelf(os, indent.GetNextIndent());
  if (this->GetRepresentation())
    {
    os << indent << "TreeRingActor: " << endl;
    this->TreeRingActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "LabelActor: " << endl;
    this->LabelActor->PrintSelf(os, indent.GetNextIndent());
    }
}

