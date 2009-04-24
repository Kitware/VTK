/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeLayoutView.cxx

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
#include "vtkTreeLayoutView.h"

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkEdgeListIterator.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkKdTreeSelector.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkTreeLayoutStrategy.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkViewTheme.h"
#include "vtkHardwareSelector.h"

#include <vtksys/stl/set>

using vtksys_stl::set;

vtkCxxRevisionMacro(vtkTreeLayoutView, "1.16");
vtkStandardNewMacro(vtkTreeLayoutView);
//----------------------------------------------------------------------------
vtkTreeLayoutView::vtkTreeLayoutView()
{
  this->Coordinate             = vtkCoordinate::New();
  this->GraphLayout            = vtkGraphLayout::New();
  this->TreeStrategy           = vtkTreeLayoutStrategy::New();
  this->GraphToPolyData        = vtkGraphToPolyData::New();
  this->VertexGlyph            = vtkVertexGlyphFilter::New();
  this->VertexMapper           = vtkPolyDataMapper::New();
  this->VertexColorLUT         = vtkLookupTable::New();
  this->VertexActor            = vtkActor::New();
  this->OutlineMapper          = vtkPolyDataMapper::New();
  this->OutlineActor           = vtkActor::New();
  this->EdgeMapper             = vtkPolyDataMapper::New();
  this->EdgeColorLUT           = vtkLookupTable::New();
  this->EdgeActor              = vtkActor::New();
  this->LabelMapper            = vtkDynamic2DLabelMapper::New();
  this->LabelActor             = vtkActor2D::New();
  this->HardwareSelector       = vtkHardwareSelector::New();
  this->KdTreeSelector         = vtkKdTreeSelector::New();
  this->ExtractSelectedGraph   = vtkExtractSelectedGraph::New();
  this->SelectionToPolyData    = vtkGraphToPolyData::New();
  this->SelectionVertexGlyph   = vtkVertexGlyphFilter::New();
  this->SelectionVertexMapper  = vtkPolyDataMapper::New();
  this->SelectionVertexActor   = vtkActor::New();
  this->SelectionEdgeMapper    = vtkPolyDataMapper::New();
  this->SelectionEdgeActor     = vtkActor::New();
  
  this->VertexColorArrayNameInternal = 0;
  this->EdgeColorArrayNameInternal = 0;
  
  // Replace the interactor style.
  vtkInteractorStyleRubberBand2D* style = vtkInteractorStyleRubberBand2D::New();
  this->SetInteractorStyle(style);
  style->Delete();
  
  // Setup view
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  this->InteractorStyle->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
  this->Coordinate->SetCoordinateSystemToDisplay();
  
  // Setup representation
  this->VertexMapper->SetScalarModeToUsePointData();
  this->VertexMapper->SetLookupTable(this->VertexColorLUT);
  this->VertexActor->PickableOff();
  this->VertexActor->GetProperty()->SetPointSize(5);
  this->OutlineActor->PickableOff();
  this->OutlineActor->GetProperty()->SetPointSize(7);
  this->OutlineActor->SetPosition(0, 0, -0.001);
  this->OutlineMapper->SetScalarVisibility(false);
  this->EdgeMapper->SetScalarModeToUseCellData();
  this->EdgeMapper->SetLookupTable(this->EdgeColorLUT);
  this->EdgeActor->SetPosition(0, 0, -0.003);
  this->LabelMapper->SetLabelModeToLabelFieldData();
  this->LabelMapper->GetLabelTextProperty()->SetColor(1,1,1);
  this->LabelMapper->GetLabelTextProperty()->SetJustificationToLeft();
  this->LabelMapper->GetLabelTextProperty()->SetVerticalJustificationToCentered();
  this->LabelMapper->GetLabelTextProperty()->SetFontSize(12);
  this->LabelMapper->GetLabelTextProperty()->SetItalic(0);
  this->LabelMapper->GetLabelTextProperty()->SetLineOffset(10);
  this->LabelActor->PickableOff();
  this->SelectionVertexActor->GetProperty()->SetPointSize(11);
  this->SelectionVertexActor->PickableOff();
  this->SelectionVertexActor->SetPosition(0, 0, -0.002);
  this->SelectionVertexMapper->SetScalarVisibility(false);
  this->SelectionEdgeActor->PickableOff();
  this->SelectionEdgeActor->SetPosition(0, 0, -0.002);
  this->SelectionEdgeMapper->SetScalarVisibility(false);
  
  // Set default parameters
  this->SetLabelArrayName("label");
  this->LabelVisibilityOff();
  this->SetVertexColorArrayName("color");
  this->ColorVerticesOff();
  this->SetEdgeColorArrayName("color");
  this->ColorEdgesOff();
  
  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();
  
  // Connect pipeline
  this->GraphLayout->SetLayoutStrategy(this->TreeStrategy);
  this->GraphToPolyData->SetInputConnection(this->GraphLayout->GetOutputPort());
  this->VertexGlyph->SetInputConnection(this->GraphToPolyData->GetOutputPort());
  this->VertexMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->VertexActor->SetMapper(this->VertexMapper);
  this->OutlineMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->EdgeMapper->SetInputConnection(this->GraphToPolyData->GetOutputPort());
  this->EdgeActor->SetMapper(this->EdgeMapper);
  this->LabelMapper->SetInputConnection(this->GraphToPolyData->GetOutputPort());
  this->LabelActor->SetMapper(this->LabelMapper);

  this->KdTreeSelector->SetInputConnection(this->GraphLayout->GetOutputPort());
  this->ExtractSelectedGraph->SetInputConnection(0, this->GraphLayout->GetOutputPort());
  vtkSmartPointer<vtkSelection> empty = vtkSmartPointer<vtkSelection>::New();
  vtkSmartPointer<vtkSelectionNode> node = vtkSmartPointer<vtkSelectionNode>::New();
  node->SetContentType(vtkSelectionNode::INDICES);
  vtkSmartPointer<vtkIdTypeArray> arr = vtkSmartPointer<vtkIdTypeArray>::New();
  node->SetSelectionList(arr);
  this->ExtractSelectedGraph->SetInput(1, empty);
  empty->AddNode(node);
  
  this->SelectionToPolyData->SetInputConnection(this->ExtractSelectedGraph->GetOutputPort());
  this->SelectionVertexGlyph->SetInputConnection(this->SelectionToPolyData->GetOutputPort());
  this->SelectionVertexMapper->SetInputConnection(this->SelectionVertexGlyph->GetOutputPort());
  this->SelectionVertexActor->SetMapper(this->SelectionVertexMapper);
  this->SelectionEdgeMapper->SetInputConnection(this->SelectionToPolyData->GetOutputPort());
  this->SelectionEdgeActor->SetMapper(this->SelectionEdgeMapper);
}

//----------------------------------------------------------------------------
vtkTreeLayoutView::~vtkTreeLayoutView()
{
  this->Coordinate->Delete();
  this->GraphLayout->Delete();
  this->TreeStrategy->Delete();
  this->GraphToPolyData->Delete();
  this->VertexGlyph->Delete();
  this->VertexMapper->Delete();
  this->VertexColorLUT->Delete();
  this->VertexActor->Delete();
  this->OutlineMapper->Delete();
  this->OutlineActor->Delete();
  this->EdgeMapper->Delete();
  this->EdgeColorLUT->Delete();
  this->EdgeActor->Delete();
  this->LabelMapper->Delete();
  this->LabelActor->Delete();
  this->KdTreeSelector->Delete();
  this->HardwareSelector->Delete();
  this->ExtractSelectedGraph->Delete();
  this->SelectionToPolyData->Delete();
  this->SelectionVertexGlyph->Delete();
  this->SelectionVertexMapper->Delete();
  this->SelectionVertexActor->Delete();
  this->SelectionEdgeMapper->Delete();
  this->SelectionEdgeActor->Delete();
  
  this->SetVertexColorArrayNameInternal(0);
  this->SetEdgeColorArrayNameInternal(0);
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetLabelArrayName(const char *name)
{
  this->LabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeLayoutView::GetLabelArrayName()
{
  return this->LabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetLabelVisibility(bool vis)
{
  this->LabelActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeLayoutView::GetLabelVisibility()
{
  return this->LabelActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::LabelVisibilityOn()
{
  this->LabelActor->SetVisibility(true);
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::LabelVisibilityOff()
{
  this->LabelActor->SetVisibility(false);
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetVertexColorArrayName(const char *name)
{
  this->SetVertexColorArrayNameInternal(name);
  this->VertexMapper->SetScalarModeToUsePointFieldData();
  this->VertexMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeLayoutView::GetVertexColorArrayName()
{
  return this->GetVertexColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetColorVertices(bool vis)
{
  this->VertexMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeLayoutView::GetColorVertices()
{
  return this->VertexMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::ColorVerticesOn()
{
  this->VertexMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::ColorVerticesOff()
{
  this->VertexMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetEdgeColorArrayName(const char *name)
{
  this->SetEdgeColorArrayNameInternal(name);
  this->EdgeMapper->SetScalarModeToUseCellFieldData();
  this->EdgeMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeLayoutView::GetEdgeColorArrayName()
{
  return this->GetEdgeColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetColorEdges(bool vis)
{
  this->EdgeMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeLayoutView::GetColorEdges()
{
  return this->EdgeMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::ColorEdgesOn()
{
  this->EdgeMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::ColorEdgesOff()
{
  this->EdgeMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetAngle(double angle)
{
  this->TreeStrategy->SetAngle(angle);
}

//----------------------------------------------------------------------------
double vtkTreeLayoutView::GetAngle()
{
  return this->TreeStrategy->GetAngle();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetRadial(bool radial)
{
  this->TreeStrategy->SetRadial(radial);
}

//----------------------------------------------------------------------------
bool vtkTreeLayoutView::GetRadial()
{
  return this->TreeStrategy->GetRadial();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::RadialOn()
{
  this->TreeStrategy->RadialOn();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::RadialOff()
{
  this->TreeStrategy->RadialOff();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetLogSpacingValue(double value)
{
  this->TreeStrategy->SetLogSpacingValue(value);
}

//----------------------------------------------------------------------------
double vtkTreeLayoutView::GetLogSpacingValue()
{
  return this->TreeStrategy->GetLogSpacingValue();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetLeafSpacing(double value)
{
  this->TreeStrategy->SetLeafSpacing(value);
}

//----------------------------------------------------------------------------
double vtkTreeLayoutView::GetLeafSpacing()
{
  return this->TreeStrategy->GetLeafSpacing();
}

//----------------------------------------------------------------------------
const char* vtkTreeLayoutView::GetDistanceArrayName()
{
  return this->TreeStrategy->GetDistanceArrayName();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetDistanceArrayName(const char *name)
{
  this->TreeStrategy->SetDistanceArrayName(name);
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::SetupRenderWindow(vtkRenderWindow *win)
{
  this->Superclass::SetupRenderWindow(win);
  win->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
}

// These should go into AddToView, RemoveFromView, SetupInputConnections
//----------------------------------------------------------------------------
void vtkTreeLayoutView::AddInputConnection(
  vtkAlgorithmOutput *conn, vtkAlgorithmOutput *selectionConn)
{
  if (this->GraphLayout->GetNumberOfInputConnections(0) == 0)
    {
    this->GraphLayout->SetInputConnection(conn);
    if (selectionConn)
      {
      this->ExtractSelectedGraph->SetInputConnection(1, selectionConn);
      }
    else
      {
      vtkSmartPointer<vtkSelection> empty =
        vtkSmartPointer<vtkSelection>::New();
      vtkSmartPointer<vtkSelectionNode> node =
        vtkSmartPointer<vtkSelectionNode>::New();
      node->SetContentType(vtkSelectionNode::INDICES);
      vtkSmartPointer<vtkIdTypeArray> arr =
        vtkSmartPointer<vtkIdTypeArray>::New();
      node->SetSelectionList(arr);
      empty->AddNode(node);
      this->ExtractSelectedGraph->SetInput(1, empty);
      }
  
    this->Renderer->AddActor(this->VertexActor);
    this->Renderer->AddActor(this->OutlineActor);
    this->Renderer->AddActor(this->EdgeActor);
    this->Renderer->AddActor(this->LabelActor);
    this->Renderer->AddActor(this->SelectionVertexActor);
    this->Renderer->AddActor(this->SelectionEdgeActor);
    this->Renderer->ResetCamera();
    }
  else
    {
    vtkErrorMacro("This view only supports one representation.");
    return;
    }
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::RemoveInputConnection(
  vtkAlgorithmOutput *conn, vtkAlgorithmOutput *selectionConn)
{
  if (this->GraphLayout->GetNumberOfInputConnections(0) > 0 &&
      this->GraphLayout->GetInputConnection(0, 0) == conn)
    {
    this->GraphLayout->RemoveInputConnection(0, conn);
    this->ExtractSelectedGraph->RemoveInputConnection(1, selectionConn);
  
    this->Renderer->RemoveActor(this->VertexActor);
    this->Renderer->RemoveActor(this->OutlineActor);
    this->Renderer->RemoveActor(this->EdgeActor);
    this->Renderer->RemoveActor(this->LabelActor);
    this->Renderer->RemoveActor(this->SelectionVertexActor);
    this->Renderer->RemoveActor(this->SelectionEdgeActor);
    }
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::MapToXYPlane(
  double displayX, double displayY, 
  double &x, double &y)
{
  this->Coordinate->SetViewport(this->Renderer);
  this->Coordinate->SetValue(displayX, displayY);
  double *pt = this->Coordinate->GetComputedWorldValue(0);

  vtkCamera *camera = this->Renderer->GetActiveCamera();
  double cameraPos[3];
  camera->GetPosition(cameraPos);

  double t = -cameraPos[2] / (pt[2] - cameraPos[2]);
  double r[3];
  for (vtkIdType i = 0; i < 3; i++)
    {
    r[i] = cameraPos[i] + t*(pt[i] - cameraPos[i]);
    }
  x = r[0];
  y = r[1];
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::ProcessEvents(
  vtkObject *caller, 
  unsigned long eventId, 
  void *callData)
{
  if (caller == this->InteractorStyle && eventId == vtkCommand::SelectionChangedEvent
      && this->GraphLayout->GetNumberOfInputConnections(0) > 0)
    {
    // Create the selection
    unsigned int *rect = reinterpret_cast<unsigned int*>(callData);
    bool singleSelectMode = false;
    unsigned int pos1X = rect[0];
    unsigned int pos1Y = rect[1];
    unsigned int pos2X = rect[2];
    unsigned int pos2Y = rect[3];
    int stretch = 2;
    if (pos1X == pos2X && pos1Y == pos2Y)
      {
      singleSelectMode = true;
      pos1X = pos1X - stretch > 0 ? pos1X - stretch : 0;
      pos1Y = pos1Y - stretch > 0 ? pos1Y - stretch : 0;
      pos2X = pos2X + stretch;
      pos2Y = pos2Y + stretch;
      }
    double pt1[2];
    double pt2[2];
    this->MapToXYPlane(pos1X, pos1Y, pt1[0], pt1[1]);
    this->MapToXYPlane(pos2X, pos2Y, pt2[0], pt2[1]);
    double minX = pt1[0] < pt2[0] ? pt1[0] : pt2[0];
    double maxX = pt1[0] < pt2[0] ? pt2[0] : pt1[0];
    double minY = pt1[1] < pt2[1] ? pt1[1] : pt2[1];
    double maxY = pt1[1] < pt2[1] ? pt2[1] : pt1[1];
    this->KdTreeSelector->SetSelectionBounds(
      minX, maxX, minY, maxY, -1, 1);
    this->KdTreeSelector->SetSingleSelection(singleSelectMode);
    double radiusX = 2*(maxX-minX);
    double radiusY = 2*(maxY-minY);
    double dist2 = radiusX*radiusX + radiusY*radiusY;
    this->KdTreeSelector->SetSingleSelectionThreshold(dist2);
    this->KdTreeSelector->Update();
    vtkSelection *kdSelection = this->KdTreeSelector->GetOutput();
    this->GraphLayout->Update();
    vtkDataObject *data = this->GraphLayout->GetOutput();
    vtkSmartPointer<vtkSelection> selection;
    selection.TakeReference(vtkConvertSelection::ToSelectionType(
      kdSelection, data, this->SelectionType, this->SelectionArrayNames));
    
#if 0
    // If the selection is empty, do a visible cell selection
    // to attempt to pick up an edge.
    vtkAbstractArray *list = selection->GetSelectionList();
    if (list->GetNumberOfTuples() == 0)
      {
      // The edge actor must be opaque for visible cell selector.
      double opacity = this->EdgeActor->GetProperty()->GetOpacity();
      this->EdgeActor->GetProperty()->SetOpacity(1);
      
      unsigned int screenMinX = pos1X < pos2X ? pos1X : pos2X;
      unsigned int screenMaxX = pos1X < pos2X ? pos2X : pos1X;
      unsigned int screenMinY = pos1Y < pos2Y ? pos1Y : pos2Y;
      unsigned int screenMaxY = pos1Y < pos2Y ? pos2Y : pos1Y;
      this->HardwareSelector->SetRenderer(this->Renderer);
      //cerr << "area=" << screenMinX << "," << screenMaxX << "," << screenMinY << "," << screenMaxY << endl;
      this->HardwareSelector->SetArea(screenMinX, screenMinY, screenMaxX, screenMaxY);
      this->HardwareSelector->SetProcessorId(0);
      this->HardwareSelector->SetRenderPasses(0, 0, 0, 0, 1);
      this->HardwareSelector->Select();  
      vtkSmartPointer<vtkIdTypeArray> ids = 
        vtkSmartPointer<vtkIdTypeArray>::New();
      this->HardwareSelector->GetSelectedIds(ids);
      
      // Set the opacity back to the original value.
      this->EdgeActor->GetProperty()->SetOpacity(opacity);
      
      vtkGraph *graph = this->GraphLayout->GetOutput();

      // Extract edge ids from the list given by the visible cell selector.
      vtkSmartPointer<vtkIdTypeArray> selectedEdges = 
        vtkSmartPointer<vtkIdTypeArray>::New();
      for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); i++)
        {
        selectedEdges->InsertNextValue(ids->GetValue(4*i+3));
        }

      // Convert edge ids to vertex ids.
      set<vtkIdType> selectedVertexSet;
      vtkSmartPointer<vtkEdgeListIterator> edges = 
        vtkSmartPointer<vtkEdgeListIterator>::New();
      graph->GetEdges(edges);
      while (edges->HasNext())
        {
        vtkEdgeType e = edges->Next();
        if (selectedEdges->LookupValue(e.Id) >= 0)
          {
          selectedVertexSet.insert(e.Source);
          if (singleSelectMode)
            {
            break;
            }
          }
        }

      vtkSmartPointer<vtkIdTypeArray> selectedVertices = 
        vtkSmartPointer<vtkIdTypeArray>::New();
      vtksys_stl::set<vtkIdType>::iterator it = selectedVertexSet.begin();
      vtksys_stl::set<vtkIdType>::iterator itEnd = selectedVertexSet.end();
      for (; it != itEnd; ++it)
        {
        selectedVertices->InsertNextValue(*it);
        }
  
      selection->Delete();
      selection = vtkSelection::New();
      selection->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
      selection->GetProperties()->Set(vtkSelection::FIELD_TYPE(), vtkSelection::POINT);
      selection->SetSelectionList(selectedVertices);
      }
#endif
    
    // If this is a union selection, append the selection
    if (rect[4] == vtkInteractorStyleRubberBand2D::SELECT_UNION)
      {
      vtkSelection* oldSelection = this->GetRepresentation()->GetSelectionLink()->GetSelection();
      selection->Union(oldSelection);
      }
    
    // Call select on the representation
    this->GetRepresentation()->Select(this, selection);
    }
  else
    {
    Superclass::ProcessEvents(caller, eventId, callData);
    }
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::PrepareForRendering()
{
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    return;
    }
  
  // Make sure the input connection is up to date.
  vtkAlgorithmOutput* conn = rep->GetInputConnection();
  vtkAlgorithmOutput* selectionConn = rep->GetSelectionConnection();
  if (this->GraphLayout->GetInputConnection(0, 0) != conn ||
      this->ExtractSelectedGraph->GetInputConnection(1, 0) != selectionConn)
    {
    this->RemoveInputConnection(
      this->GraphLayout->GetInputConnection(0, 0),
      this->ExtractSelectedGraph->GetInputConnection(1, 0));
    this->AddInputConnection(conn, selectionConn);
    }

  // Update the pipeline up until the graph to polydata
  this->GraphToPolyData->Update();
  vtkPolyData* pd = this->GraphToPolyData->GetOutput();
  
  // Try to find the range the user-specified color array.
  // If we cannot find that array, use the scalar range.
  double range[2];
  vtkDataArray* arr = 0; 
  if (this->GetColorEdges())
    {
    if (this->GetEdgeColorArrayName())
      {
      arr = pd->GetCellData()->GetArray(this->GetEdgeColorArrayName());
      }
    if (!arr)
      {
      arr = pd->GetCellData()->GetScalars();
      }
    if (arr)
      {
      arr->GetRange(range);    
      this->EdgeMapper->SetScalarRange(range[0], range[1]);
      }
    }

  // Do the same thing for the vertex array.
  arr = 0; 
  if (this->GetColorVertices())
    {
    if (this->GetVertexColorArrayName())
      {
      arr = pd->GetPointData()->GetArray(this->GetVertexColorArrayName());
      }
    if (!arr)
      {
      arr = pd->GetPointData()->GetScalars();
      }
    if (arr)
      {
      arr->GetRange(range);    
      this->VertexMapper->SetScalarRange(range[0], range[1]);
      }
    }
  
  this->Superclass::PrepareForRendering();
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  
  this->VertexActor->GetProperty()->SetColor(theme->GetPointColor());
  this->OutlineActor->GetProperty()->SetColor(theme->GetOutlineColor());
  this->VertexColorLUT->SetHueRange(theme->GetPointHueRange()); 
  this->VertexColorLUT->SetSaturationRange(theme->GetPointSaturationRange()); 
  this->VertexColorLUT->SetValueRange(theme->GetPointValueRange()); 
  this->VertexColorLUT->SetAlphaRange(theme->GetPointAlphaRange()); 
  this->VertexColorLUT->Build();

  this->LabelMapper->GetLabelTextProperty()->SetColor(theme->GetVertexLabelColor());

  this->EdgeActor->GetProperty()->SetColor(theme->GetCellColor());
  this->EdgeActor->GetProperty()->SetOpacity(theme->GetCellOpacity());
  this->EdgeColorLUT->SetHueRange(theme->GetCellHueRange()); 
  this->EdgeColorLUT->SetSaturationRange(theme->GetCellSaturationRange()); 
  this->EdgeColorLUT->SetValueRange(theme->GetCellValueRange()); 
  this->EdgeColorLUT->SetAlphaRange(theme->GetCellAlphaRange()); 
  this->EdgeColorLUT->Build();

  this->SelectionEdgeActor->GetProperty()->SetColor(theme->GetSelectedCellColor());
  this->SelectionEdgeActor->GetProperty()->SetOpacity(theme->GetSelectedCellOpacity());
  this->SelectionVertexActor->GetProperty()->SetColor(theme->GetSelectedPointColor());
  this->SelectionVertexActor->GetProperty()->SetOpacity(theme->GetSelectedPointOpacity());
}

//----------------------------------------------------------------------------
void vtkTreeLayoutView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Coordinate: " << endl;
  this->Coordinate->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphLayout: " << endl;
  this->GraphLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeStrategy: " << endl;
  this->TreeStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphToPolyData: " << endl;
  this->GraphToPolyData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexGlyph: " << endl;
  this->VertexGlyph->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexMapper: " << endl;
  this->VertexMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexColorLUT: " << endl;
  this->VertexColorLUT->PrintSelf(os, indent.GetNextIndent());
  os << indent << "OutlineMapper: " << endl;
  this->OutlineMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeMapper: " << endl;
  this->EdgeMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeColorLUT: " << endl;
  this->EdgeColorLUT->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LabelMapper: " << endl;
  this->LabelMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "KdTreeSelector: " << endl;
  this->KdTreeSelector->PrintSelf(os, indent.GetNextIndent());
  os << indent << "HardwareSelector: " << endl;
  this->HardwareSelector->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ExtractSelectedGraph: " << endl;
  this->ExtractSelectedGraph->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionToPolyData: " << endl;
  this->SelectionToPolyData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionVertexGlyph: " << endl;
  this->SelectionVertexGlyph->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionVertexMapper: " << endl;
  this->SelectionVertexMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionEdgeMapper: " << endl;
  this->SelectionEdgeMapper->PrintSelf(os, indent.GetNextIndent());
  if (this->GetRepresentation())
    {
    os << indent << "VertexActor: " << endl;
    this->VertexActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "OutlineActor: " << endl;
    this->OutlineActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeActor: " << endl;
    this->EdgeActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "LabelActor: " << endl;
    this->LabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "SelectionVertexActor: " << endl;
    this->SelectionVertexActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "SelectionEdgeActor: " << endl;
    this->SelectionEdgeActor->PrintSelf(os, indent.GetNextIndent());
    }
}

