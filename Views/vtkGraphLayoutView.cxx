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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkGraphLayoutView.h"

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkCommand.h"
#include "vtkDataRepresentation.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGraphLayout.h"
#include "vtkGraphToPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkKdTreeSelector.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPassThroughLayoutStrategy.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRandomLayoutStrategy.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkClustering2DLayoutStrategy.h"
#include "vtkCommunity2DLayoutStrategy.h"
#include "vtkConstrained2DLayoutStrategy.h"
#include "vtkTextProperty.h"
#include "vtkVertexDegree.h"
#include "vtkCellCenters.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkViewTheme.h"
#include "vtkVisibleCellSelector.h"

#include <ctype.h> // for tolower()

vtkCxxRevisionMacro(vtkGraphLayoutView, "1.10");
vtkStandardNewMacro(vtkGraphLayoutView);
//----------------------------------------------------------------------------
vtkGraphLayoutView::vtkGraphLayoutView()
{
  this->Coordinate             = vtkCoordinate::New();
  this->GraphLayout            = vtkGraphLayout::New();
  this->RandomStrategy         = vtkRandomLayoutStrategy::New();
  this->Simple2DStrategy       = vtkSimple2DLayoutStrategy::New();
  this->Clustering2DStrategy   = vtkClustering2DLayoutStrategy::New();
  this->Community2DStrategy    = vtkCommunity2DLayoutStrategy::New();
  this->Constrained2DStrategy  = vtkConstrained2DLayoutStrategy::New();
  this->Fast2DStrategy         = vtkFast2DLayoutStrategy::New();
  this->ForceDirectedStrategy  = vtkForceDirectedLayoutStrategy::New();
  this->PassThroughStrategy    = vtkPassThroughLayoutStrategy::New();
  this->CircularStrategy       = vtkCircularLayoutStrategy::New();
  this->VertexDegree           = vtkVertexDegree::New();
  this->CellCenters            = vtkCellCenters::New();
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
  this->VertexLabelMapper      = vtkDynamic2DLabelMapper::New();
  this->VertexLabelActor       = vtkActor2D::New();
  this->EdgeLabelMapper        = vtkDynamic2DLabelMapper::New();
  this->EdgeLabelActor         = vtkActor2D::New();
  this->VisibleCellSelector    = vtkVisibleCellSelector::New();
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
  this->LayoutStrategyInternal = 0;
  this->SelectionArrayNameInternal = 0;
  
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
  this->VertexLabelMapper->SetLabelModeToLabelFieldData();
  this->VertexLabelMapper->GetLabelTextProperty()->SetColor(1,1,1);
  this->VertexLabelMapper->GetLabelTextProperty()->SetJustificationToCentered();
  this->VertexLabelMapper->GetLabelTextProperty()->SetVerticalJustificationToCentered();
  this->VertexLabelMapper->GetLabelTextProperty()->SetFontSize(12);
  this->VertexLabelMapper->GetLabelTextProperty()->SetItalic(0);
  this->VertexLabelMapper->GetLabelTextProperty()->SetLineOffset(-10);
  this->VertexLabelMapper->SetPriorityArrayName("VertexDegree");
  this->VertexLabelActor->PickableOff();
  this->EdgeLabelMapper->SetLabelModeToLabelFieldData();
  this->EdgeLabelMapper->GetLabelTextProperty()->SetColor(.7,.7,1);
  this->EdgeLabelMapper->GetLabelTextProperty()->SetJustificationToCentered();
  this->EdgeLabelMapper->GetLabelTextProperty()->SetVerticalJustificationToCentered();
  this->EdgeLabelMapper->GetLabelTextProperty()->SetFontSize(10);
  this->EdgeLabelMapper->GetLabelTextProperty()->SetItalic(0);
  this->EdgeLabelMapper->GetLabelTextProperty()->SetLineOffset(-10);
  this->EdgeLabelActor->PickableOff();
  this->SelectionVertexActor->GetProperty()->SetPointSize(11);
  this->SelectionVertexActor->PickableOff();
  this->SelectionVertexActor->SetPosition(0, 0, -0.002);
  this->SelectionVertexMapper->SetScalarVisibility(false);
  this->SelectionEdgeActor->PickableOff();
  this->SelectionEdgeActor->SetPosition(0, 0, -0.002);
  this->SelectionEdgeMapper->SetScalarVisibility(false);
  
  // Set default parameters
  this->SetVertexLabelArrayName("label");
  this->VertexLabelVisibilityOff();
  this->SetEdgeLabelArrayName("label");
  this->EdgeLabelVisibilityOff();
  this->SetVertexColorArrayName("VertexDegree");
  this->ColorVerticesOff();
  this->SetEdgeColorArrayName("weight");
  this->ColorEdgesOff();
  this->SetLayoutStrategyToSimple2D();
  
  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();
  
  // Connect pipeline
  this->GraphLayout->SetLayoutStrategy(this->Simple2DStrategy);
  this->VertexDegree->SetInputConnection(this->GraphLayout->GetOutputPort());
  this->CellCenters->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->GraphToPolyData->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->VertexGlyph->SetInputConnection(this->GraphToPolyData->GetOutputPort());
  this->VertexMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->VertexActor->SetMapper(this->VertexMapper);
  this->OutlineMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->OutlineActor->SetMapper(this->OutlineMapper);
  this->EdgeMapper->SetInputConnection(this->GraphToPolyData->GetOutputPort());
  this->EdgeActor->SetMapper(this->EdgeMapper);
  this->VertexLabelMapper->SetInputConnection(this->GraphToPolyData->GetOutputPort());
  this->VertexLabelActor->SetMapper(this->VertexLabelMapper);
  this->EdgeLabelMapper->SetInputConnection(this->CellCenters->GetOutputPort());
  this->EdgeLabelActor->SetMapper(this->EdgeLabelMapper);

  this->KdTreeSelector->SetInputConnection(this->GraphLayout->GetOutputPort());
  this->ExtractSelectedGraph->SetInputConnection(0, this->GraphLayout->GetOutputPort());
  vtkSelection* empty = vtkSelection::New();
  empty->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
  vtkIdTypeArray* arr = vtkIdTypeArray::New();
  empty->SetSelectionList(arr);
  arr->Delete();
  this->ExtractSelectedGraph->SetInput(1, empty);
  empty->Delete();
  
  this->SelectionToPolyData->SetInputConnection(this->ExtractSelectedGraph->GetOutputPort());
  this->SelectionVertexGlyph->SetInputConnection(this->SelectionToPolyData->GetOutputPort());
  this->SelectionVertexMapper->SetInputConnection(this->SelectionVertexGlyph->GetOutputPort());
  this->SelectionVertexActor->SetMapper(this->SelectionVertexMapper);
  this->SelectionEdgeMapper->SetInputConnection(this->SelectionToPolyData->GetOutputPort());
  this->SelectionEdgeActor->SetMapper(this->SelectionEdgeMapper);
}

//----------------------------------------------------------------------------
vtkGraphLayoutView::~vtkGraphLayoutView()
{
  this->Coordinate->Delete();
  this->GraphLayout->Delete();
  this->RandomStrategy->Delete();
  this->Fast2DStrategy->Delete();
  this->Simple2DStrategy->Delete();
  this->ForceDirectedStrategy->Delete();
  this->Clustering2DStrategy->Delete();
  this->Community2DStrategy->Delete();
  this->Constrained2DStrategy->Delete();
  this->PassThroughStrategy->Delete();
  this->CircularStrategy->Delete();
  this->VertexDegree->Delete();
  this->CellCenters->Delete();
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
  this->VertexLabelMapper->Delete();
  this->VertexLabelActor->Delete();
  this->EdgeLabelMapper->Delete();
  this->EdgeLabelActor->Delete();
  this->KdTreeSelector->Delete();
  this->VisibleCellSelector->Delete();
  this->ExtractSelectedGraph->Delete();
  this->SelectionToPolyData->Delete();
  this->SelectionVertexGlyph->Delete();
  this->SelectionVertexMapper->Delete();
  this->SelectionVertexActor->Delete();
  this->SelectionEdgeMapper->Delete();
  this->SelectionEdgeActor->Delete();
  
  this->SetVertexColorArrayNameInternal(0);
  this->SetEdgeColorArrayNameInternal(0);
  this->SetLayoutStrategyInternal(0);
  this->SetSelectionArrayNameInternal(0);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexLabelArrayName(const char* name)
{
  this->VertexLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetVertexLabelArrayName()
{
  return this->VertexLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLabelArrayName(const char* name)
{
  this->EdgeLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEdgeLabelArrayName()
{
  return this->EdgeLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexLabelVisibility(bool vis)
{
  this->VertexLabelActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetVertexLabelVisibility()
{
  return this->VertexLabelActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::VertexLabelVisibilityOn()
{
  this->VertexLabelActor->SetVisibility(true);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::VertexLabelVisibilityOff()
{
  this->VertexLabelActor->SetVisibility(false);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeLabelVisibility(bool vis)
{
  this->EdgeLabelActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetEdgeLabelVisibility()
{
  return this->EdgeLabelActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::EdgeLabelVisibilityOn()
{
  this->EdgeLabelActor->SetVisibility(true);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::EdgeLabelVisibilityOff()
{
  this->EdgeLabelActor->SetVisibility(false);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetVertexColorArrayName(const char* name)
{
  this->SetVertexColorArrayNameInternal(name);
  this->VertexMapper->SetScalarModeToUsePointFieldData();
  this->VertexMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetVertexColorArrayName()
{
  return this->GetVertexColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetColorVertices(bool vis)
{
  this->VertexMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetColorVertices()
{
  return this->VertexMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::ColorVerticesOn()
{
  this->VertexMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::ColorVerticesOff()
{
  this->VertexMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetEdgeColorArrayName(const char* name)
{
  this->SetEdgeColorArrayNameInternal(name);
  this->EdgeMapper->SetScalarModeToUseCellFieldData();
  this->EdgeMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetEdgeColorArrayName()
{
  return this->GetEdgeColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetColorEdges(bool vis)
{
  this->EdgeMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkGraphLayoutView::GetColorEdges()
{
  return this->EdgeMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::ColorEdgesOn()
{
  this->EdgeMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::ColorEdgesOff()
{
  this->EdgeMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetSelectionArrayName(const char* name)
{
  this->SetSelectionArrayNameInternal(name);
  this->KdTreeSelector->SetSelectionFieldName(name);
}

//----------------------------------------------------------------------------
const char* vtkGraphLayoutView::GetSelectionArrayName()
{
  return this->GetSelectionArrayNameInternal();
}

//----------------------------------------------------------------------------
int vtkGraphLayoutView::IsLayoutComplete()
{
  if (this->GraphLayout)
    {
    return this->GraphLayout->IsLayoutComplete();
    }
    
  // If I don't have a strategy I guess it's
  // better to say I'm done than not done :)
  return 1;
}
  
//----------------------------------------------------------------------------
void vtkGraphLayoutView::UpdateLayout()
{
  // See if the graph layout is complete
  // if it's not then set it as modified
  // and call a render on the render window
  if (!this->IsLayoutComplete())
    {
    this->GraphLayout->Modified();
    if (this->GetRenderWindow())
      {
      this->Renderer->ResetCamera();
      this->GetRenderWindow()->Render();
      }
    }
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetLayoutStrategy(const char* name)
{
  vtkGraphLayoutStrategy* s = this->Simple2DStrategy;
  if (!name)
    {
    return;
    }
  
  // Take out spaces and make lowercase.
  char str[20];
  strncpy(str, name, 20);
  int pos = 0;
  for (int i = 0; str[i] != '\0' && i < 20; i++, pos++)
    {
    if (str[i] == ' ')
      {
      pos--;
      }
    else
      {
      str[pos] = tolower(str[i]);
      }
    }
  str[pos] = '\0';
  
  if (!strcmp(str, "random"))
    {
    s = this->RandomStrategy;
    }
  else if (!strcmp(str, "forcedirected"))
    {
    s = this->ForceDirectedStrategy;
    }
  else if (!strcmp(str, "simple2d"))
    {
    s = this->Simple2DStrategy;
    }
  else if (!strcmp(str, "clustering2d"))
    {
    s = this->Clustering2DStrategy;
    }
  else if (!strcmp(str, "community2d"))
    {
    s = this->Community2DStrategy;
    }
  else if (!strcmp(str, "constrained2d"))
    {
    s = this->Constrained2DStrategy;
    }
  else if (!strcmp(str, "fast2d"))
    {
    s = this->Fast2DStrategy;
    }
  else if (!strcmp(str, "passthrough"))
    {
    s = this->PassThroughStrategy;
    }
  else if (!strcmp(str, "circular"))
    {
    s = this->CircularStrategy;
    }
  else
    {
    vtkErrorMacro("Unknown strategy " << name << " (" << str << ").");
    return;
    }
  this->GraphLayout->SetLayoutStrategy(s);
  this->SetLayoutStrategyInternal(name);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetIterationsPerLayout(int iterations)
{
  // Hmmm... this seems a bit silly, probably a better way
  vtkGraphLayoutStrategy* strategy = this->GraphLayout->GetLayoutStrategy();
  vtkSimple2DLayoutStrategy *simple = 
    vtkSimple2DLayoutStrategy::SafeDownCast(strategy);
  vtkFast2DLayoutStrategy *fast = 
    vtkFast2DLayoutStrategy::SafeDownCast(strategy);
  if (simple)
    {
    simple->SetIterationsPerLayout(iterations);
    }
  else if (fast)
    {
    fast->SetIterationsPerLayout(iterations);
    }
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetupRenderWindow(vtkRenderWindow* win)
{
  this->Superclass::SetupRenderWindow(win);
  win->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::AddInputConnection(vtkAlgorithmOutput* conn)
{
  if (this->GraphLayout->GetNumberOfInputConnections(0) == 0)
    {
    this->GraphLayout->SetInputConnection(conn);
  
    this->Renderer->AddActor(this->VertexActor);
    this->Renderer->AddActor(this->OutlineActor);
    this->Renderer->AddActor(this->EdgeActor);
    this->Renderer->AddActor(this->VertexLabelActor);
    this->Renderer->AddActor(this->EdgeLabelActor);
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
void vtkGraphLayoutView::RemoveInputConnection(vtkAlgorithmOutput* conn)
{
  if (this->GraphLayout->GetNumberOfInputConnections(0) > 0 &&
      this->GraphLayout->GetInputConnection(0, 0) == conn)
    {
    this->GraphLayout->RemoveInputConnection(0, conn);
  
    this->Renderer->RemoveActor(this->VertexActor);
    this->Renderer->RemoveActor(this->OutlineActor);
    this->Renderer->RemoveActor(this->EdgeActor);
    this->Renderer->RemoveActor(this->VertexLabelActor);
    this->Renderer->RemoveActor(this->EdgeLabelActor);
    this->Renderer->RemoveActor(this->SelectionVertexActor);
    this->Renderer->RemoveActor(this->SelectionEdgeActor);
    }
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::SetSelectionLink(vtkSelectionLink* link)
{
  this->ExtractSelectedGraph->SetInputConnection(1, link->GetOutputPort());
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::MapToXYPlane(
  double displayX, double displayY, 
  double &x, double &y)
{
  this->Coordinate->SetViewport(this->Renderer);
  this->Coordinate->SetValue(displayX, displayY);
  double* pt = this->Coordinate->GetComputedWorldValue(0);

  vtkCamera* camera = this->Renderer->GetActiveCamera();
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
void vtkGraphLayoutView::ProcessEvents(
  vtkObject* caller, 
  unsigned long eventId, 
  void* callData)
{
  if (caller == this->InteractorStyle && eventId == vtkCommand::SelectionChangedEvent
      && this->GraphLayout->GetNumberOfInputConnections(0) > 0)
    {
    // Create the selection
    unsigned int* rect = reinterpret_cast<unsigned int*>(callData);
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
    vtkSelection* selection = this->KdTreeSelector->GetOutput();
    selection->Register(0);
    
    // If the selection is empty, do a visible cell selection
    // to attempt to pick up an edge.
    vtkAbstractArray* list = selection->GetSelectionList();
    if (list && (list->GetNumberOfTuples() == 0))
      {
      // The edge actor must be opaque for visible cell selector.
      double opacity = this->EdgeActor->GetProperty()->GetOpacity();
      this->EdgeActor->GetProperty()->SetOpacity(1);
      
      unsigned int screenMinX = pos1X < pos2X ? pos1X : pos2X;
      unsigned int screenMaxX = pos1X < pos2X ? pos2X : pos1X;
      unsigned int screenMinY = pos1Y < pos2Y ? pos1Y : pos2Y;
      unsigned int screenMaxY = pos1Y < pos2Y ? pos2Y : pos1Y;
      this->VisibleCellSelector->SetRenderer(this->Renderer);
      this->VisibleCellSelector->SetArea(screenMinX, screenMinY, screenMaxX, screenMaxY);
      this->VisibleCellSelector->SetProcessorId(0);
      this->VisibleCellSelector->SetRenderPasses(0, 0, 0, 0, 1);
      this->VisibleCellSelector->Select();  
      vtkIdTypeArray* ids = vtkIdTypeArray::New();
      this->VisibleCellSelector->GetSelectedIds(ids);
      
      // Set the opacity back to the original value.
      this->EdgeActor->GetProperty()->SetOpacity(opacity);
      
      vtkIdTypeArray* selectedIds = vtkIdTypeArray::New();
      vtkAbstractGraph* graph = this->GraphLayout->GetOutput();
      for (vtkIdType i = 0; i < ids->GetNumberOfTuples(); i++)
        {
        vtkIdType edge = ids->GetValue(4*i+3);
        vtkIdType source = graph->GetSourceVertex(edge);
        vtkIdType target = graph->GetTargetVertex(edge);
        selectedIds->InsertNextValue(source);
        selectedIds->InsertNextValue(target);
        if (singleSelectMode)
          {
          break;
          }
        }
      
      selection->Delete();
      selection = vtkSelection::New();
      selection->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
      selection->GetProperties()->Set(vtkSelection::FIELD_TYPE(), vtkSelection::POINT);
      selection->SetSelectionList(selectedIds);
      ids->Delete();
      selectedIds->Delete();      
      }
    
    // If this is a union selection, append the selection
    if (rect[4] == vtkInteractorStyleRubberBand2D::SELECT_UNION)
      {
      vtkSelection* oldSelection = this->GetRepresentation()->GetSelectionLink()->GetSelection();
      selection->Union(oldSelection);
      }
    
    // Call select on the representation(s)
    this->GetRepresentation()->Select(this, selection);
    
    selection->Delete();
    }
  else if(eventId == vtkCommand::SelectionChangedEvent)
    {
    this->Update();
    Superclass::ProcessEvents(caller, eventId, callData);
    }
  else
    {
    Superclass::ProcessEvents(caller, eventId, callData);
    }
}

//----------------------------------------------------------------------------
void vtkGraphLayoutView::PrepareForRendering()
{
  // Make sure we have a representation.
  vtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
    {
    return;
    }
  
  // Make sure the input connection is up to date.
  vtkAlgorithmOutput* conn = rep->GetInputConnection();
  if (this->GraphLayout->GetInputConnection(0, 0) != conn)
    {
    this->RemoveInputConnection(this->GraphLayout->GetInputConnection(0, 0));
    this->AddInputConnection(conn);
    }
  
  // Make sure the selection link is up to date.
  vtkSelectionLink* link = rep->GetSelectionLink();
  if (this->ExtractSelectedGraph->GetInputConnection(1, 0)->GetProducer() != link)
    {
    this->SetSelectionLink(link);
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
void vtkGraphLayoutView::ApplyViewTheme(vtkViewTheme* theme)
{
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  
  this->VertexActor->GetProperty()->SetColor(theme->GetPointColor());
  this->OutlineActor->GetProperty()->SetColor(theme->GetOutlineColor());
  this->VertexColorLUT->SetHueRange(theme->GetPointHueRange()); 
  this->VertexColorLUT->SetSaturationRange(theme->GetPointSaturationRange()); 
  this->VertexColorLUT->SetValueRange(theme->GetPointValueRange()); 
  this->VertexColorLUT->SetAlphaRange(theme->GetPointAlphaRange()); 
  this->VertexColorLUT->Build();

  this->VertexLabelMapper->GetLabelTextProperty()->
    SetColor(theme->GetVertexLabelColor());
  this->EdgeLabelMapper->GetLabelTextProperty()->
    SetColor(theme->GetEdgeLabelColor());

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
void vtkGraphLayoutView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Coordinate: " << endl;
  this->Coordinate->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphLayout: " << endl;
  this->GraphLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "RandomStrategy: " << endl;
  this->RandomStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Simple2DStrategy: " << endl;
  this->Simple2DStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Clustering2DStrategy: " << endl;
  this->Clustering2DStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Community2DStrategy: " << endl;
  this->Community2DStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Constrained2DStrategy: " << endl;
  this->Community2DStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Fast2DStrategy: " << endl;
  this->Fast2DStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ForceDirectedStrategy: " << endl;
  this->ForceDirectedStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "PassThroughStrategy: " << endl;
  this->PassThroughStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "CircularStrategy: " << endl;
  this->CircularStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphLayout: " << endl;
  this->GraphLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexDegree: " << endl;
  this->VertexDegree->PrintSelf(os, indent.GetNextIndent());
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
  os << indent << "VertexLabelMapper: " << endl;
  this->VertexLabelMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeLabelMapper: " << endl;
  this->EdgeLabelMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "KdTreeSelector: " << endl;
  this->KdTreeSelector->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VisibleCellSelector: " << endl;
  this->VisibleCellSelector->PrintSelf(os, indent.GetNextIndent());
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
  os << indent << "LayoutStrategy: "
     << (this->LayoutStrategyInternal ? this->LayoutStrategyInternal : "(null)") << endl;
  if (this->GetRepresentation())
    {
    os << indent << "VertexActor: " << endl;
    this->VertexActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "OutlineActor: " << endl;
    this->OutlineActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeActor: " << endl;
    this->EdgeActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "VertexLabelActor: " << endl;
    this->VertexLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeLabelActor: " << endl;
    this->EdgeLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "SelectionVertexActor: " << endl;
    this->SelectionVertexActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "SelectionEdgeActor: " << endl;
    this->SelectionEdgeActor->PrintSelf(os, indent.GetNextIndent());
    }
}

