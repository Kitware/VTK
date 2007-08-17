/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeMapView.cxx

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

#include "vtkTreeMapView.h"

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkBoxLayoutStrategy.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleTreeMapHover.h"
#include "vtkLabeledTreeMapDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSliceAndDiceLayoutStrategy.h"
#include "vtkSquarifyLayoutStrategy.h"
#include "vtkTextProperty.h"
#include "vtkTreeFieldAggregator.h"
#include "vtkTreeLevelsFilter.h"
#include "vtkTreeMapLayout.h"
#include "vtkTreeMapToPolyData.h"
#include "vtkViewTheme.h"

vtkCxxRevisionMacro(vtkTreeMapView, "1.1");
vtkStandardNewMacro(vtkTreeMapView);
//----------------------------------------------------------------------------
vtkTreeMapView::vtkTreeMapView()
{
  this->ColorArrayNameInternal = 0;
  
  this->TreeLevelsFilter      = vtkTreeLevelsFilter::New();
  this->TreeFieldAggregator   = vtkTreeFieldAggregator::New();
  this->TreeMapLayout         = vtkTreeMapLayout::New();
  this->BoxLayout             = vtkBoxLayoutStrategy::New();
  this->SliceAndDiceLayout    = vtkSliceAndDiceLayoutStrategy::New();
  this->SquarifyLayout        = vtkSquarifyLayoutStrategy::New();
  this->TreeMapToPolyData     = vtkTreeMapToPolyData::New();
  this->TreeMapMapper         = vtkPolyDataMapper::New();
  this->TreeMapActor          = vtkActor::New();
  this->LabelMapper           = vtkLabeledTreeMapDataMapper::New();
  this->LabelActor            = vtkActor2D::New();
  this->ColorLUT              = vtkLookupTable::New();
  
  // Replace the interactor style
  vtkInteractorStyleTreeMapHover* style = vtkInteractorStyleTreeMapHover::New();
  this->SetInteractorStyle(style);
  style->Delete();
  
  // Set up view
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  style->SetLayout(this->TreeMapLayout);
  style->SetTreeMapToPolyData(this->TreeMapToPolyData); 
  style->AddObserver(vtkCommand::UserEvent, this->GetObserver());
  
  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();
  
  // Set up representation
  this->TreeFieldAggregator->SetLeafVertexUnitSize(false);
  this->TreeFieldAggregator->SetMinValue(1e-10);
  this->TreeFieldAggregator->SetLogScale(false);
  this->TreeMapToPolyData->SetLevelsFieldName("level");  
  this->ColorLUT->SetHueRange(0.667, 0);
  this->ColorLUT->Build();
  this->TreeMapMapper->SetLookupTable(ColorLUT);
  this->LabelMapper->SetLabelFormat("%s");
  this->LabelMapper->SetLabelModeToLabelFieldData();
  this->LabelMapper->SetClipTextMode(0);
  this->LabelActor->SetPickable(false);
  
  // Set default properties
  this->SetBorderPercentage(0.1);
  this->SetSizeArrayName("size");
  this->SetHoverArrayName("name");
  this->SetLabelArrayName("name");
  this->SetFontSizeRange(24, 10);
  this->SetLayoutStrategyToSquarify();

  // Wire pipeline
  this->TreeFieldAggregator->SetInputConnection(
    this->TreeLevelsFilter->GetOutputPort());
  this->TreeMapLayout->SetInputConnection(
    this->TreeFieldAggregator->GetOutputPort());
  this->TreeMapToPolyData->SetInputConnection(
    this->TreeMapLayout->GetOutputPort());
  this->TreeMapMapper->SetInputConnection(
    this->TreeMapToPolyData->GetOutputPort());
  this->TreeMapActor->SetMapper(this->TreeMapMapper);
  this->LabelMapper->SetInputConnection(
    this->TreeMapLayout->GetOutputPort());
  this->LabelActor->SetMapper(this->LabelMapper);
}

//----------------------------------------------------------------------------
vtkTreeMapView::~vtkTreeMapView()
{
  this->SetColorArrayNameInternal(0);
  
  // Delete objects
  this->TreeLevelsFilter->Delete();
  this->TreeFieldAggregator->Delete();
  this->TreeMapLayout->Delete();
  this->BoxLayout->Delete();
  this->SliceAndDiceLayout->Delete();
  this->SquarifyLayout->Delete();
  this->TreeMapToPolyData->Delete();
  this->TreeMapMapper->Delete();
  this->TreeMapActor->Delete();
  this->LabelMapper->Delete();
  this->LabelActor->Delete();
  this->ColorLUT->Delete();
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetBorderPercentage(double pcent)
{
  this->BoxLayout->SetBorderPercentage(pcent);
  this->SliceAndDiceLayout->SetBorderPercentage(pcent);
  this->SquarifyLayout->SetBorderPercentage(pcent);
}

//----------------------------------------------------------------------------
double vtkTreeMapView::GetBorderPercentage()
{
  return this->BoxLayout->GetBorderPercentage();
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetSizeArrayName(const char* name)
{
  this->TreeFieldAggregator->SetField(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeMapView::GetSizeArrayName()
{
  return this->TreeFieldAggregator->GetField();
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetHoverArrayName(const char* name)
{
  vtkInteractorStyleTreeMapHover::SafeDownCast(this->InteractorStyle)->SetLabelField(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeMapView::GetHoverArrayName()
{
  return vtkInteractorStyleTreeMapHover::SafeDownCast(this->InteractorStyle)->GetLabelField();
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLabelArrayName(const char* name)
{
  this->LabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeMapView::GetLabelArrayName()
{
  return this->LabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetColorArrayName(const char *field)
{
  this->TreeMapMapper->SetScalarModeToUseCellFieldData();
  this->TreeMapMapper->SelectColorArray(field);
  this->SetColorArrayNameInternal(field);
}

//----------------------------------------------------------------------------
const char* vtkTreeMapView::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLayoutStrategyToBox()
{
  this->SetLayoutStrategy("Box");
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLayoutStrategyToSliceAndDice()
{
  this->SetLayoutStrategy("Slice And Dice");
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLayoutStrategyToSquarify()
{
  this->SetLayoutStrategy("Squarify");
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetLayoutStrategy(const char* name)
{
  if (!strcmp(name, "Box"))
    {
    this->TreeMapLayout->SetLayoutStrategy(this->BoxLayout);
    }
  else if (!strcmp(name, "Slice And Dice"))
    {
    this->TreeMapLayout->SetLayoutStrategy(this->SliceAndDiceLayout);
    }
  else if (!strcmp(name, "Squarify"))
    {
    this->TreeMapLayout->SetLayoutStrategy(this->SquarifyLayout);
    }
  else
    {
    vtkErrorMacro("Unknown layout name: " << name);
    }
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetFontSizeRange(const int maxSize, const int minSize)
{
  this->LabelMapper->SetFontSizeRange(maxSize, minSize);
}

//----------------------------------------------------------------------------
void vtkTreeMapView::SetupRenderWindow(vtkRenderWindow* win)
{
  this->Superclass::SetupRenderWindow(win);
  win->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
}

//----------------------------------------------------------------------------
void vtkTreeMapView::AddInputConnection(vtkAlgorithmOutput* conn)
{
  if (this->TreeLevelsFilter->GetNumberOfInputConnections(0) == 0)
    {
    this->TreeLevelsFilter->SetInputConnection(conn);
    
    this->Renderer->AddActor(this->TreeMapActor);
    this->Renderer->AddActor(this->LabelActor);
    this->Renderer->ResetCamera();
    }
  else
    {
    vtkErrorMacro("This view only supports one representation.");
    }
}

//----------------------------------------------------------------------------
void vtkTreeMapView::RemoveInputConnection(vtkAlgorithmOutput* conn)
{
  if (this->TreeLevelsFilter->GetNumberOfInputConnections(0) > 0 &&
      this->TreeLevelsFilter->GetInputConnection(0, 0) == conn)
    {
    this->TreeLevelsFilter->RemoveInputConnection(0, conn);
  
    this->Renderer->RemoveActor(this->TreeMapActor);
    this->Renderer->RemoveActor(this->LabelActor);
    }  
}

//----------------------------------------------------------------------------
void vtkTreeMapView::ProcessEvents(
  vtkObject* caller, 
  unsigned long eventId, 
  void* callData)
{
  if (caller == this->InteractorStyle && eventId == vtkCommand::UserEvent)
    {
    // Create the selection
    vtkSelection* selection = vtkSelection::New();
    vtkIdTypeArray* list = vtkIdTypeArray::New();
    vtkIdType* id = reinterpret_cast<vtkIdType*>(callData);
    if (*id >= 0)
      {
      list->InsertNextValue(*id);
      }
    selection->SetSelectionList(list);
    list->Delete();
    // TODO: This should really be pedigree ids.
    selection->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
    
    // Call select on the representation(s)
    this->GetRepresentation()->Select(this, selection);
    
    selection->Delete();
    }
  else
    {
    Superclass::ProcessEvents(caller, eventId, callData);
    }
}

//----------------------------------------------------------------------------
void vtkTreeMapView::PrepareForRendering()
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
    this->RemoveInputConnection(this->TreeLevelsFilter->GetInputConnection(0, 0));
    this->AddInputConnection(conn);
    }
  
  // Use the most recent selection
  vtkSelection* selection = rep->GetSelectionLink()->GetSelection();
  // TODO: Should be pedigree ids.
  if (selection->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) != vtkSelection::INDICES)
    {
    vtkErrorMacro("Can only handle INDICES selections.");
    return;
    }
  vtkIdTypeArray* arr = vtkIdTypeArray::SafeDownCast(selection->GetSelectionList());
  vtkIdType id = -1;
  if (arr->GetNumberOfTuples() > 0)
    {
    id = arr->GetValue(0);
    }
  vtkInteractorStyleTreeMapHover::SafeDownCast(this->InteractorStyle)->HighLightItem(id);
  
  // Update the pipeline up until the treemap to polydata
  this->TreeMapToPolyData->Update();
  
  // Try to find the range the user-specified color array.
  // If we cannot find that array, use the scalar range. 
  double range[2];
  vtkDataArray* array = 0; 
  if (this->GetColorArrayName())
    {
    array = this->TreeMapToPolyData->GetOutput()->
      GetCellData()->GetArray(this->GetColorArrayName());
    }
  if (array)
    {
    array->GetRange(range);    
    }
  else
    {
    this->TreeMapToPolyData->GetOutput()->GetScalarRange(range);
    }
  this->TreeMapMapper->SetScalarRange(range[0], range[1]);
  
  this->Superclass::PrepareForRendering();
}

//----------------------------------------------------------------------------
void vtkTreeMapView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  
  //this->VertexActor->GetProperty()->SetColor(theme->GetPointColor());
  //this->OutlineActor->GetProperty()->SetColor(theme->GetOutlineColor());
  //this->VertexColorLUT->SetHueRange(theme->GetPointHueRange()); 
  //this->VertexColorLUT->SetSaturationRange(theme->GetPointSaturationRange()); 
  //this->VertexColorLUT->SetValueRange(theme->GetPointValueRange()); 
  //this->VertexColorLUT->SetAlphaRange(theme->GetPointAlphaRange()); 
  //this->VertexColorLUT->Build();

  this->LabelMapper->GetLabelTextProperty()->SetColor(theme->GetVertexLabelColor());

  //this->EdgeActor->GetProperty()->SetColor(theme->GetCellColor());
  //this->EdgeActor->GetProperty()->SetOpacity(theme->GetCellOpacity());
  //this->EdgeColorLUT->SetHueRange(theme->GetCellHueRange()); 
  //this->EdgeColorLUT->SetSaturationRange(theme->GetCellSaturationRange()); 
  //this->EdgeColorLUT->SetValueRange(theme->GetCellValueRange()); 
  //this->EdgeColorLUT->SetAlphaRange(theme->GetCellAlphaRange()); 
  //this->EdgeColorLUT->Build();

  //this->SelectionEdgeActor->GetProperty()->SetColor(theme->GetSelectedCellColor());
  //this->SelectionEdgeActor->GetProperty()->SetOpacity(theme->GetSelectedCellOpacity());
  double color[3];
  theme->GetSelectedPointColor(color);
  vtkInteractorStyleTreeMapHover::SafeDownCast(this->InteractorStyle)->
    SetSelectionLightColor(color[0], color[1], color[2]);
  //this->SelectionVertexActor->GetProperty()->SetOpacity(theme->GetSelectedPointOpacity());
}

//----------------------------------------------------------------------------
void vtkTreeMapView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TreeLevelsFilter: " << endl;
  this->TreeLevelsFilter->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeFieldAggregator: " << endl;
  this->TreeFieldAggregator->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeMapLayout: " << endl;
  this->TreeMapLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "BoxLayout: " << endl;
  this->BoxLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SliceAndDiceLayout: " << endl;
  this->SliceAndDiceLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SquarifyLayout: " << endl;
  this->SquarifyLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeMapToPolyData: " << endl;
  this->TreeMapToPolyData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeMapMapper: " << endl;
  this->TreeMapMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeMapActor: " << endl;
  this->TreeMapActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LabelMapper: " << endl;
  this->LabelMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LabelActor: " << endl;
  this->LabelActor->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ColorLUT: " << endl;
  this->ColorLUT->PrintSelf(os, indent.GetNextIndent());
}

