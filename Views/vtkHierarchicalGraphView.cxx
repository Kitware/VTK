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
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCircularLayoutStrategy.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkCosmicTreeLayoutStrategy.h"
#include "vtkDataRepresentation.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkEdgeCenters.h"
#include "vtkEdgeListIterator.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkGlyph3D.h"
#include "vtkGlyphSource2D.h"
#include "vtkGraphHierarchicalBundle.h"
#include "vtkGraphLayout.h"
#include "vtkGraphMapper.h"
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
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarsToColors.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSplineFilter.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkTree.h"
#include "vtkTreeFieldAggregator.h"
#include "vtkTreeLayoutStrategy.h"
#include "vtkVertexDegree.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkViewTheme.h"
#include "vtkHardwareSelector.h"

#include <ctype.h> // for tolower()

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkHierarchicalGraphView, "1.11");
vtkStandardNewMacro(vtkHierarchicalGraphView);
//----------------------------------------------------------------------------
vtkHierarchicalGraphView::vtkHierarchicalGraphView()
{
  // Processing objects
  this->Coordinate             = vtkSmartPointer<vtkCoordinate>::New();
  this->GraphLayout            = vtkSmartPointer<vtkGraphLayout>::New();
  this->TreeStrategy           = vtkSmartPointer<vtkTreeLayoutStrategy>::New();
  this->CosmicTreeStrategy     = vtkSmartPointer<vtkCosmicTreeLayoutStrategy>::New();
  this->CircularStrategy       = vtkSmartPointer<vtkCircularLayoutStrategy>::New();
  this->PassThroughStrategy    = vtkSmartPointer<vtkPassThroughLayoutStrategy>::New();
  this->VertexDegree           = vtkSmartPointer<vtkVertexDegree>::New();
  this->EdgeCenters            = vtkSmartPointer<vtkEdgeCenters>::New();
  this->TreeAggregation        = vtkSmartPointer<vtkTreeFieldAggregator>::New();
  this->ExtractSelectedTree    = vtkSmartPointer<vtkExtractSelectedGraph>::New();
  this->SelectedTreeMapper     = vtkSmartPointer<vtkGraphMapper>::New();
  this->SelectedTreeActor      = vtkSmartPointer<vtkActor>::New();
  
  // Representation objects
  this->TreeMapper             = vtkSmartPointer<vtkGraphMapper>::New();
  this->TreeActor              = vtkSmartPointer<vtkActor>::New();
  this->VertexLabelMapper      = vtkSmartPointer<vtkDynamic2DLabelMapper>::New();
  this->VertexLabelActor       = vtkSmartPointer<vtkActor2D>::New();
  this->EdgeLabelMapper        = vtkSmartPointer<vtkDynamic2DLabelMapper>::New();
  this->EdgeLabelActor         = vtkSmartPointer<vtkActor2D>::New();
  this->HBundle                = vtkSmartPointer<vtkGraphHierarchicalBundle>::New();
  this->Spline                 = vtkSmartPointer<vtkSplineFilter>::New();
  this->GraphEdgeMapper        = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->GraphEdgeActor         = vtkSmartPointer<vtkActor>::New();
  this->TreeVisibilityRepresentation = vtkSmartPointer<vtkDataRepresentation>::New();
  
  // Selection objects
  this->HardwareSelector       = vtkSmartPointer<vtkHardwareSelector>::New();
  this->KdTreeSelector         = vtkSmartPointer<vtkKdTreeSelector>::New();
  this->ExtractSelectedGraph   = vtkSmartPointer<vtkExtractSelectedGraph>::New();
  this->SelectedGraphHBundle   = vtkSmartPointer<vtkGraphHierarchicalBundle>::New();
  this->SelectedGraphSpline    = vtkSmartPointer<vtkSplineFilter>::New();
  this->SelectedGraphMapper    = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->SelectedGraphActor     = vtkSmartPointer<vtkActor>::New();
  
  vtkGraphLayoutStrategy *nothing = 0;
  this->SetLayoutStrategy(nothing);
  this->LayoutStrategyNameInternal = 0;
  this->IconArrayNameInternal = 0;

  // Replace the interactor style.
  vtkInteractorStyleRubberBand2D* style = vtkInteractorStyleRubberBand2D::New();
  this->SetInteractorStyle(style);
  style->Delete();
  
  // Setup view
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  this->InteractorStyle->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());
  this->Coordinate->SetCoordinateSystemToDisplay();
  
  // Setup parameters on the various mappers and actors
  this->VertexLabelMapper->SetLabelModeToLabelFieldData();
  this->VertexLabelMapper->GetLabelTextProperty()->SetColor(1,1,1);
  this->VertexLabelMapper->GetLabelTextProperty()->SetJustificationToCentered();
  this->VertexLabelMapper->GetLabelTextProperty()->SetVerticalJustificationToCentered();
  this->VertexLabelMapper->GetLabelTextProperty()->SetFontSize(12);
  this->VertexLabelMapper->GetLabelTextProperty()->SetItalic(0);
  this->VertexLabelMapper->GetLabelTextProperty()->SetLineOffset(-10);
  this->VertexLabelMapper->SetPriorityArrayName("leaf_count");
  this->VertexLabelActor->PickableOff();
  this->EdgeLabelMapper->SetLabelModeToLabelFieldData();
  this->EdgeLabelMapper->GetLabelTextProperty()->SetColor(.7,.7,1);
  this->EdgeLabelMapper->GetLabelTextProperty()->SetJustificationToCentered();
  this->EdgeLabelMapper->GetLabelTextProperty()->SetVerticalJustificationToCentered();
  this->EdgeLabelMapper->GetLabelTextProperty()->SetFontSize(10);
  this->EdgeLabelMapper->GetLabelTextProperty()->SetItalic(0);
  this->EdgeLabelMapper->GetLabelTextProperty()->SetLineOffset(-10);
  this->EdgeLabelMapper->SetPriorityArrayName("weight");
  this->EdgeLabelActor->PickableOff();
  this->SelectedGraphActor->PickableOff();
  this->SelectedGraphActor->SetPosition(0, 0, -0.01);
  this->SelectedGraphMapper->SetScalarVisibility(false);
  this->SelectedTreeActor->PickableOff();
  this->SelectedTreeActor->SetPosition(0, 0, -0.01);
  this->SelectedTreeMapper->SetScalarVisibility(false);
  
  // By default both the tree and selected tree 
  // edges are OFF
  this->TreeMapper->SetEdgeVisibility(false);
  this->SelectedTreeMapper->SetEdgeVisibility(false);
  
  // Set default parameters
  this->SetVertexLabelArrayName("id");
  this->VertexLabelVisibilityOff();
  this->SetEdgeLabelArrayName("id");
  this->EdgeLabelVisibilityOff();
  this->ColorVerticesOff();
  this->ColorEdgesOff();
  
  // Misc variables
  this->Radial = false;
  this->Angle = 120;
  this->LogSpacing = 1.2;
  this->LeafSpacing = .5;
  this->BundlingStrength = .5;
  
  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();

  // Make empty seleciton for default highlight
  this->EmptySelection = vtkSmartPointer<vtkSelection>::New();
  vtkSmartPointer<vtkSelectionNode> node = vtkSmartPointer<vtkSelectionNode>::New();
  node->GetProperties()->Set(vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::INDICES);
  vtkSmartPointer<vtkIdTypeArray> arr = vtkSmartPointer<vtkIdTypeArray>::New();
  node->SetSelectionList(arr);
  this->EmptySelection->AddNode(node);

  // Set filter attributes
  this->TreeAggregation->LeafVertexUnitSizeOn();
  this->TreeAggregation->SetField("leaf_count");
  this->GraphLayout->SetLayoutStrategy(this->TreeStrategy);
  this->HBundle->SetBundlingStrength(this->BundlingStrength);
  this->SelectedGraphHBundle->SetBundlingStrength(this->BundlingStrength);
  this->Spline->SetMaximumNumberOfSubdivisions(16);
  
  // Connect pipeline:
  //
  // TreeRepresentation* + + + + + + + + + + + + 
  //    |                                       +
  // TreeAgg                                    +
  //    |                                       +
  // GraphLayout                                +
  //    |                                       +
  // VertexDegree --------------------------- ExtractSelectedTree
  //    |                                       |
  //    +--------------------- TreeMapper   SelectedTreeMapper
  //    |                          |            |
  //    | GraphRepresentation**  TreeActor    SelectedTreeActor
  //    |         |    +  |
  //    |         |    +  |
  // GraphToTree  |    +  |
  //          |   |    +  |
  //          |   |    +  |
  //          |   |   ExtractSelectedGraph
  //          |   |                 |
  //         HBundle             SelectedGraphHBundle
  //            |                   |
  //         Spline              SelectedGraphSpline
  //            |                   |
  //         GraphMapper         SelectedGraphMapper
  //            |                   |
  //         GraphActor          SelectedGraphActor
  //
  // *  - The TreeRepresentation is retrieved with GetRepresentation(0,0)
  // ** - The GraphRepresentation is retrieved with GetRepresentation(1,0)
  // +  - Selection connection
  // -  - Data connection
  //
  this->GraphLayout->SetInputConnection(this->TreeAggregation->GetOutputPort());
  this->VertexDegree->SetInputConnection(this->GraphLayout->GetOutputPort());
  this->HBundle->SetInputConnection(1, this->VertexDegree->GetOutputPort(0));
  this->Spline->SetInputConnection(0, this->HBundle->GetOutputPort(0));
  this->TreeMapper->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->TreeActor->SetMapper(this->TreeMapper);
  this->VertexLabelMapper->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->VertexLabelActor->SetMapper(this->VertexLabelMapper);
  this->EdgeCenters->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->EdgeLabelMapper->SetInputConnection(this->EdgeCenters->GetOutputPort());
  this->EdgeLabelActor->SetMapper(this->EdgeLabelMapper);
  this->GraphEdgeMapper->SetInputConnection(this->Spline->GetOutputPort());
  this->GraphEdgeActor->SetMapper(this->GraphEdgeMapper);
  this->KdTreeSelector->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->ExtractSelectedGraph->SetInput(1, this->EmptySelection);
  this->SelectedGraphHBundle->SetInputConnection(0, this->ExtractSelectedGraph->GetOutputPort());
  this->SelectedGraphHBundle->SetInputConnection(1, this->VertexDegree->GetOutputPort());
  this->SelectedGraphSpline->SetInputConnection(this->SelectedGraphHBundle->GetOutputPort());
  this->SelectedGraphMapper->SetInputConnection(this->SelectedGraphSpline->GetOutputPort());
  this->SelectedGraphActor->SetMapper(this->SelectedGraphMapper);
  this->SelectedGraphActor->GetProperty()->SetLineWidth(5.0);
  this->ExtractSelectedTree->SetInputConnection(0, this->VertexDegree->GetOutputPort());
  this->ExtractSelectedTree->SetInput(1, this->EmptySelection);
  this->SelectedTreeMapper->SetInputConnection(this->ExtractSelectedTree->GetOutputPort());
  this->SelectedTreeActor->SetMapper(this->SelectedTreeMapper);
  
  // Okay set up the tree layout strategies
  this->TreeStrategy->SetRadial(this->Radial);
  this->TreeStrategy->SetAngle(this->Angle);
  this->TreeStrategy->SetLogSpacingValue(this->LogSpacing);
  this->TreeStrategy->SetLeafSpacing(this->LeafSpacing);

  this->TreeVisibilityRepresentation->AddObserver(
    vtkCommand::SelectionChangedEvent, this->GetObserver());

  // Register any algorithm that can fire progress events with the superclass.
  this->RegisterProgress(this->TreeAggregation, "TreeAggregation");
  this->RegisterProgress(this->GraphLayout, "GraphLayout");
  this->RegisterProgress(this->VertexDegree, "VertexDegree");
  this->RegisterProgress(this->HBundle, "HBundle");
  this->RegisterProgress(this->Spline, "Spline");
  this->RegisterProgress(this->GraphEdgeMapper, "CurvedEdgeMapper");
}

//----------------------------------------------------------------------------
vtkHierarchicalGraphView::~vtkHierarchicalGraphView()
{  
  // Delete internally created objects.
  // Note: All of the smartpointer objects 
  //       will be deleted for us
  vtkGraphLayoutStrategy *nothing = 0;
  this->SetLayoutStrategy(nothing);
  this->SetLayoutStrategyNameInternal(0);
  this->SetIconArrayNameInternal(0);

  // UnRegister any algorithm that can fire progress events from the superclass.
  this->UnRegisterProgress(this->TreeAggregation);
  this->UnRegisterProgress(this->GraphLayout);
  this->UnRegisterProgress(this->VertexDegree);
  this->UnRegisterProgress(this->HBundle);
  this->UnRegisterProgress(this->Spline);
  this->UnRegisterProgress(this->GraphEdgeMapper);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetHierarchyFromInputConnection(vtkAlgorithmOutput* conn)
{
  return this->SetRepresentationFromInputConnection(0, conn);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetHierarchyFromInput(vtkDataObject* input)
{
  return this->SetRepresentationFromInput(0, input);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetGraphFromInputConnection(vtkAlgorithmOutput* conn)
{
  return this->SetRepresentationFromInputConnection(1, conn);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::SetGraphFromInput(vtkDataObject* input)
{
  return this->SetRepresentationFromInput(1, input);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalGraphView::GetTreeVisibilityRepresentation()
{
  return this->TreeVisibilityRepresentation;
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetVertexLabelArrayName(const char* name)
{
  this->VertexLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetVertexLabelArrayName()
{
  return this->VertexLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetEdgeLabelArrayName(const char* name)
{
  this->EdgeLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetEdgeLabelArrayName()
{
  return this->EdgeLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetVertexLabelVisibility(bool vis)
{
  this->VertexLabelActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetVertexLabelVisibility()
{
  return this->VertexLabelActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::VertexLabelVisibilityOn()
{
  this->VertexLabelActor->SetVisibility(true);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::VertexLabelVisibilityOff()
{
  this->VertexLabelActor->SetVisibility(false);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetEdgeLabelVisibility(bool vis)
{
  this->EdgeLabelActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetEdgeLabelVisibility()
{
  return this->EdgeLabelActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::EdgeLabelVisibilityOn()
{
  this->EdgeLabelActor->SetVisibility(true);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::EdgeLabelVisibilityOff()
{
  this->EdgeLabelActor->SetVisibility(false);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetVertexColorArrayName(const char* name)
{
  this->TreeMapper->SetVertexColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetVertexColorArrayName()
{
  return this->TreeMapper->GetVertexColorArrayName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetColorVertices(bool vis)
{
  this->TreeMapper->SetColorVertices(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetColorVertices()
{
  return this->TreeMapper->GetColorVertices();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::ColorVerticesOn()
{
  this->TreeMapper->ColorVerticesOn();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::ColorVerticesOff()
{
  this->TreeMapper->ColorVerticesOff();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetScaledGlyphs(bool arg)
{
  this->TreeMapper->SetScaledGlyphs(arg);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetScaledGlyphs()
{
  return this->TreeMapper->GetScaledGlyphs();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetScalingArrayName(const char* name)
{
  this->CosmicTreeStrategy->SetNodeSizeArrayName(name);
  this->CosmicTreeStrategy->SetSizeLeafNodesOnly(true);
  
  // Okay the cosmic tree strategy creates a new array
  // with the scaling array + "TreeRadius" so that's what
  // we'll use for our scaling for the mapper
  vtkStdString treeRadiusArrayName(name);
  treeRadiusArrayName + "TreeRadius";
  this->TreeMapper->SetScalingArrayName( treeRadiusArrayName );
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetScalingArrayName()
{
  return this->TreeMapper->GetScalingArrayName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetEdgeColorArrayName(const char* name)
{   
  // Try to find the range the user-specified color array.
  double range[2];
  vtkDataArray* arr = 0; 
  arr = this->Spline->GetOutput()->GetCellData()->GetArray(name);
  if (arr)
    {
    this->GraphEdgeMapper->SetScalarModeToUseCellFieldData();
    this->GraphEdgeMapper->SelectColorArray(name); 
    arr->GetRange(range);  
    this->GraphEdgeMapper->SetScalarRange(range[0], range[1]);
    }
  else
    {
    vtkErrorMacro("Could not find array named: " << name);
    }
}

void vtkHierarchicalGraphView::SetEdgeColorToSplineFraction()
{
  // FIXME: For some odd reason this causes a lockup.

  // Try to find the range the fraction color array.
  double range[2];
  vtkDataArray* arr = 0; 
  arr = this->Spline->GetOutput()->GetPointData()->GetArray("fraction");
  if (arr)
    {
    this->GraphEdgeMapper->SetScalarModeToUsePointFieldData();
    this->GraphEdgeMapper->SelectColorArray("fraction");
    arr->GetRange(range);    
    this->GraphEdgeMapper->SetScalarRange(range[0], range[1]);
    }
  else
    {
    vtkErrorMacro("Could not find spline fraction array");
    }
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetEdgeColorArrayName()
{
  return this->GraphEdgeMapper->GetArrayName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetColorEdges(bool vis)
{
  this->GraphEdgeMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetColorEdges()
{
  return this->GraphEdgeMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::ColorEdgesOn()
{
  this->GraphEdgeMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::ColorEdgesOff()
{
  this->GraphEdgeMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetTreeEdgeVisibility(bool vis)
{
  this->TreeMapper->SetEdgeVisibility(vis);
  this->SelectedTreeMapper->SetEdgeVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetTreeEdgeVisibility()
{
  return this->TreeMapper->GetEdgeVisibility();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::TreeEdgeVisibilityOn()
{
  this->TreeMapper->SetEdgeVisibility(true);
  this->SelectedTreeMapper->SetEdgeVisibility(true);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::TreeEdgeVisibilityOff()
{
  this->TreeMapper->SetEdgeVisibility(false);
  this->SelectedTreeMapper->SetEdgeVisibility(false);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetLayoutStrategy(vtkGraphLayoutStrategy *s)
{
  this->LayoutStrategy = s;
  this->GraphLayout->SetLayoutStrategy(this->LayoutStrategy);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetLayoutStrategy(const char* name)
{
  this->LayoutStrategy = this->TreeStrategy;
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
  
  if (!strcmp(str, "tree"))
    {
    this->LayoutStrategy = this->TreeStrategy;
    }
  else if (!strcmp(str, "cosmictree"))
    {
    this->LayoutStrategy = this->CosmicTreeStrategy;
    }
  else
    {
    vtkErrorMacro("Unknown strategy " << name << " (" << str << ").");
    return;
    }
  this->GraphLayout->SetLayoutStrategy(this->LayoutStrategy);
  this->SetLayoutStrategyNameInternal(name);
}


//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetIconArrayName(const char* name)
{
  this->SetIconArrayNameInternal(name);
  this->TreeMapper->SetIconArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalGraphView::GetIconArrayName()
{
  return this->GetIconArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::AddIconType(char *type, int index)
{
  this->TreeMapper->AddIconType(type, index);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::ClearIconTypes()
{
  this->TreeMapper->ClearIconTypes();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetIconTexture(vtkTexture *texture)
{
  this->TreeMapper->SetIconTexture(texture);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetIconSize(int *size)
{
  this->TreeMapper->SetIconSize(size);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetIconAlignment(int alignment)
{
  this->TreeMapper->SetIconAlignment(alignment);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetIconVisibility(bool b)
{
  this->TreeMapper->SetIconVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalGraphView::GetIconVisibility()
{
  return this->TreeMapper->GetIconVisibility();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::SetupRenderWindow(vtkRenderWindow* win)
{
  this->Superclass::SetupRenderWindow(win);
  win->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
  this->Renderer->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::AddInputConnection( int port, int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* selectionConn)
{
  vtkAlgorithm* alg = conn->GetProducer();
  alg->Update();

  bool haveGraph = false;
  bool haveTree = false;

  //Port 0 is designated as the tree and port 1 is the graph
  if( port == 0 )
    {
    this->TreeAggregation->SetInputConnection(0, conn);
    if (selectionConn)
      {
      this->ExtractSelectedTree->SetInputConnection(1, selectionConn);
      }
    else
      {
      this->ExtractSelectedTree->SetInput(1, this->EmptySelection);
      }
    haveTree = true;
    }
  else
    {
    this->HBundle->SetInputConnection(0, conn);
    this->ExtractSelectedGraph->SetInputConnection(0, conn);
    if (selectionConn)
      {
      this->ExtractSelectedGraph->SetInputConnection(1, selectionConn);
      }
    else
      {
      this->ExtractSelectedGraph->SetInput(1, this->EmptySelection);
      }
    haveGraph = true;
    }

  haveGraph = haveGraph || this->GetGraphRepresentation();
  haveTree = haveTree || this->GetTreeRepresentation();

  // If we have a graph and a tree, we are ready to go.
  if (haveGraph && haveTree)
    {
    this->Renderer->AddActor(this->TreeActor);
    this->Renderer->AddActor(this->SelectedGraphActor);
    this->Renderer->AddActor(this->SelectedTreeActor);
    this->Renderer->AddActor(this->VertexLabelActor);
    this->Renderer->AddActor(this->EdgeLabelActor);
    this->Renderer->AddActor(this->GraphEdgeActor);
    this->Renderer->ResetCamera();
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::RemoveInputConnection( int port, int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* selectionConn)
{
  if( port == 0 )
  {
    if (this->TreeAggregation->GetNumberOfInputConnections(0) > 0 &&
        this->TreeAggregation->GetInputConnection(0, 0) == conn)
    {
      this->TreeAggregation->RemoveInputConnection(0, conn);
      this->ExtractSelectedTree->RemoveInputConnection(1, selectionConn);
    }
  }
  else if( port == 1 )
  {
    if (this->HBundle->GetNumberOfInputConnections(0) > 0 &&
        this->HBundle->GetInputConnection(0, 0) == conn)
    {
      this->HBundle->RemoveInputConnection(0, conn);
      this->ExtractSelectedGraph->RemoveInputConnection(0, conn);
      this->ExtractSelectedGraph->RemoveInputConnection(1, selectionConn);
    }
  }
  
  this->Renderer->RemoveActor(this->TreeActor);
  this->Renderer->RemoveActor(this->SelectedGraphActor);
  this->Renderer->RemoveActor(this->SelectedTreeActor);
  this->Renderer->RemoveActor(this->VertexLabelActor);
  this->Renderer->RemoveActor(this->EdgeLabelActor);
  this->Renderer->RemoveActor(this->GraphEdgeActor);
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::MapToXYPlane(
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
void vtkHierarchicalGraphView::ProcessEvents(
  vtkObject* caller, 
  unsigned long eventId, 
  void* callData)
{
  if (caller == this->InteractorStyle && eventId == vtkCommand::SelectionChangedEvent
      && this->GraphLayout->GetNumberOfInputConnections(0) > 0)
    {
    // First make sure the view is set up.
    vtkDataRepresentation* treeRep = this->GetRepresentation();
    vtkDataRepresentation* graphRep = this->GetRepresentation(1, 0);
      
    if (!treeRep || !graphRep)
      {
      return;
      }

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
    vtkSelection* kdSelection = this->KdTreeSelector->GetOutput();

    // Convert to the proper selection type. Must be pedigree ids for this view.
    this->GraphLayout->Update();
    vtkSmartPointer<vtkSelection> vertexSelection;
    vertexSelection.TakeReference(vtkConvertSelection::ToSelectionType(
      kdSelection, this->GraphLayout->GetOutput(), vtkSelectionNode::PEDIGREEIDS));

    vtkSmartPointer<vtkSelection> selection = vtkSmartPointer<vtkSelection>::New();

    if (kdSelection->GetNode(0)->GetSelectionList()->GetNumberOfTuples() > 0)
      {
      selection->ShallowCopy(vertexSelection);
      }
    else
      {
      // If we didn't find any vertices, perform edge selection.
      // The edge actor must be opaque for visible cell selection.
      int scalarVis = this->GraphEdgeMapper->GetScalarVisibility();
      this->GraphEdgeMapper->ScalarVisibilityOff();
      vtkScalarsToColors* lookup = this->GraphEdgeMapper->GetLookupTable();
      lookup->Register(0);
      this->GraphEdgeMapper->SetLookupTable(0);
      double opacity = this->GraphEdgeActor->GetProperty()->GetOpacity();
      this->GraphEdgeActor->GetProperty()->SetOpacity(1.0);
      
      unsigned int screenMinX = pos1X < pos2X ? pos1X : pos2X;
      unsigned int screenMaxX = pos1X < pos2X ? pos2X : pos1X;
      unsigned int screenMinY = pos1Y < pos2Y ? pos1Y : pos2Y;
      unsigned int screenMaxY = pos1Y < pos2Y ? pos2Y : pos1Y;
      this->HardwareSelector->SetRenderer(this->Renderer);
      this->HardwareSelector->SetArea(screenMinX, screenMinY, screenMaxX, screenMaxY);
      this->HardwareSelector->SetFieldAssociation(
        vtkDataObject::FIELD_ASSOCIATION_CELLS);
      vtkSmartPointer<vtkSelection> sel;
      sel.TakeReference(this->HardwareSelector->Select());
      vtkSmartPointer<vtkIdTypeArray> ids;
      if (sel)
        {
        vtkSelectionNode* node = sel->GetNode(0);
        if (node && node->GetProperties()->Get(vtkSelectionNode::PROP()) ==
            this->GraphEdgeMapper)
          {
          ids = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
          }
        }

      // Set the speciale dge actor back to normal.
      this->GraphEdgeMapper->SetScalarVisibility(scalarVis);
      this->GraphEdgeMapper->SetLookupTable(lookup);
      lookup->Delete();
      this->GraphEdgeActor->GetProperty()->SetOpacity(opacity);
      
      vtkSmartPointer<vtkIdTypeArray> selectedIds = vtkSmartPointer<vtkIdTypeArray>::New();
      for (vtkIdType i = 0; ids && (i < ids->GetNumberOfTuples()); i++)
        {
        vtkIdType edge = ids->GetValue(i);
        selectedIds->InsertNextValue(edge);
        if (singleSelectMode)
          {
          break;
          }
        }
      
      // Start with polydata cell selection of lines.
      vtkSmartPointer<vtkSelection> cellIndexSelection = vtkSmartPointer<vtkSelection>::New();
      vtkSmartPointer<vtkSelectionNode> cellIndexSelectionNode = vtkSmartPointer<vtkSelectionNode>::New();
      cellIndexSelection->AddNode(cellIndexSelectionNode);
      cellIndexSelectionNode->SetContentType(vtkSelectionNode::INDICES);
      cellIndexSelectionNode->SetFieldType(vtkSelectionNode::CELL);
      cellIndexSelectionNode->SetSelectionList(selectedIds);

      // Convert to pedigree ids.
      this->HBundle->Update();
      vtkSmartPointer<vtkSelection> edgeSelection;
      edgeSelection.TakeReference(vtkConvertSelection::ToSelectionType(
        cellIndexSelection, this->HBundle->GetOutput(), vtkSelectionNode::PEDIGREEIDS));

      // Make it an edge selection.
      edgeSelection->GetNode(0)->SetFieldType(vtkSelectionNode::EDGE);

      if (cellIndexSelectionNode->GetSelectionList()->GetNumberOfTuples() > 0)
        {
        selection->ShallowCopy(edgeSelection);
        }
      }

    // If this is a union selection, append the selection
    if (rect[4] == vtkInteractorStyleRubberBand2D::SELECT_UNION)
      {
      vtkSelection* oldSelection =
        this->GetRepresentation()->GetSelectionLink()->GetSelection();
      selection->Union(oldSelection);
      }
    
    // Call select on the representation(s)
    for (int i = 0; i < 2; ++i)
      {
      for (int j = 0; j < this->GetNumberOfRepresentations(i); ++j)
        {
        this->GetRepresentation(i, j)->Select(this, selection);
        }
      }
    }
  else
    {
    Superclass::ProcessEvents(caller, eventId, callData);
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::PrepareForRendering()
{
  if (!this->GetTreeRepresentation() || !this->GetGraphRepresentation())
    {
    return;
    }
  // Make sure the tree input connection is up to date.
  vtkDataRepresentation* treeRep = this->GetRepresentation();
  
  vtkAlgorithmOutput* treeConn = treeRep->GetInputConnection();
  vtkAlgorithmOutput* selectionConn = treeRep->GetSelectionConnection();
  if (this->TreeAggregation->GetInputConnection(0, 0) != treeConn ||
      this->ExtractSelectedTree->GetInputConnection(1, 0) != selectionConn)
    {
    this->RemoveInputConnection(0, 0,
      this->TreeAggregation->GetInputConnection(0, 0),
      this->ExtractSelectedTree->GetInputConnection(1, 0));
    this->AddInputConnection(0, 0, treeConn, selectionConn);
    }

  // Make sure the graph input connection is up to date.
  vtkDataRepresentation* graphRep = this->GetRepresentation(1, 0);
  
  vtkAlgorithmOutput* graphConn = graphRep->GetInputConnection();
  selectionConn = graphRep->GetSelectionConnection();
  if (this->HBundle->GetInputConnection(0, 0) != graphConn ||
      this->ExtractSelectedGraph->GetInputConnection(1, 0) != selectionConn)
    {
    this->RemoveInputConnection(1, 0,
      this->HBundle->GetInputConnection(0, 0),
      this->ExtractSelectedGraph->GetInputConnection(1, 0));
    this->AddInputConnection(1, 0, graphConn, selectionConn);
    }
  
  this->Superclass::PrepareForRendering();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::ApplyViewTheme(vtkViewTheme* theme)
{

  // Take some parameters from the theme and apply
  // to objects within this class
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  this->Renderer->SetBackground2(theme->GetBackgroundColor2());
  this->Renderer->SetGradientBackground(true);
  
  this->VertexLabelMapper->GetLabelTextProperty()->
    SetColor(theme->GetVertexLabelColor());
  this->EdgeLabelMapper->GetLabelTextProperty()->
    SetColor(theme->GetEdgeLabelColor());
    
  // Pass theme to the graph mapper
  this->TreeMapper->ApplyViewTheme(theme);
  
  // Set vertex size and edge size on mapper
  this->TreeMapper->SetVertexPointSize(theme->GetPointSize());
  this->TreeMapper->SetEdgeLineWidth(theme->GetLineWidth());
  
  
  // Pull selection info from theme, create a new theme, 
  // and pass to the selection graph mapper
  vtkViewTheme *selectTheme = vtkViewTheme::New();
  selectTheme->SetPointColor(theme->GetSelectedPointColor());
  selectTheme->SetCellColor(theme->GetSelectedCellColor());
  selectTheme->SetOutlineColor(theme->GetSelectedPointColor());
  this->SelectedTreeMapper->ApplyViewTheme(selectTheme);
  
  // Set vertex size and edge size on mapper
  this->SelectedTreeMapper->SetVertexPointSize(theme->GetPointSize()+2);
  this->SelectedTreeMapper->SetEdgeLineWidth(theme->GetLineWidth()+1);
  
  selectTheme->Delete();
  
  // Now apply theme to the curved edges
  VTK_CREATE(vtkLookupTable,lut);
  this->GraphEdgeActor->GetProperty()->SetLineWidth(theme->GetLineWidth());
  this->GraphEdgeActor->GetProperty()->SetColor(theme->GetCellColor());
  this->GraphEdgeActor->GetProperty()->SetOpacity(theme->GetCellOpacity());
  lut->SetHueRange(theme->GetCellHueRange()); 
  lut->SetSaturationRange(theme->GetCellSaturationRange()); 
  lut->SetValueRange(theme->GetCellValueRange()); 
  lut->SetAlphaRange(theme->GetCellAlphaRange()); 
  lut->Build();
  this->GraphEdgeMapper->SetLookupTable(lut);
}

void vtkHierarchicalGraphView::SetVertexLabelFontSize(const int size)
{
  this->VertexLabelMapper->GetLabelTextProperty()->SetFontSize(size);
}

int vtkHierarchicalGraphView::GetVertexLabelFontSize()
{
  return this->VertexLabelMapper->GetLabelTextProperty()->GetFontSize();
}

void vtkHierarchicalGraphView::SetEdgeLabelFontSize(const int size)
{
  this->EdgeLabelMapper->GetLabelTextProperty()->SetFontSize(size);
}

int vtkHierarchicalGraphView::GetEdgeLabelFontSize()
{
  return this->EdgeLabelMapper->GetLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkHierarchicalGraphView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Coordinate: " << endl;
  this->Coordinate->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphLayout: " << endl;
  this->GraphLayout->PrintSelf(os, indent.GetNextIndent());
  if (this->LayoutStrategy)
    {
    os << indent << "LayoutStrategy: " << endl;
    this->LayoutStrategy->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "TreeStrategy: " << endl;
  this->TreeStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "CosmicTreeStrategy: " << endl;
  this->CosmicTreeStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "PassThroughStrategy: " << endl;
  this->PassThroughStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "CircularStrategy: " << endl;
  this->CircularStrategy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexDegree: " << endl;
  this->VertexDegree->PrintSelf(os, indent.GetNextIndent());
  os << indent << "TreeMapper: " << endl;
  this->TreeMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectedGraphMapper: " << endl;
  this->SelectedGraphMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexLabelMapper: " << endl;
  this->VertexLabelMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeLabelMapper: " << endl;
  this->EdgeLabelMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphMapper: " << endl;
  this->GraphEdgeMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "KdTreeSelector: " << endl;
  this->KdTreeSelector->PrintSelf(os, indent.GetNextIndent());
  os << indent << "HardwareSelector: " << endl;
  this->HardwareSelector->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ExtractSelectedGraph: " << endl;
  this->ExtractSelectedGraph->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectedGraphHBundle: " << endl;
  this->SelectedGraphHBundle->PrintSelf(os, indent.GetNextIndent());
    
  if (this->GetGraphRepresentation() && this->GetTreeRepresentation())
    {
    os << indent << "VertexLabelActor: " << endl;
    this->VertexLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeLabelActor: " << endl;
    this->EdgeLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "GraphActor: " << endl;
    this->GraphEdgeActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "TreeActor: " << endl;
    this->TreeActor->PrintSelf(os, indent.GetNextIndent());
    }
}

// ----------------------------------------------------------------------

void
vtkHierarchicalGraphView::SetLeafSpacing(double spacing)
{
  this->TreeStrategy->SetLeafSpacing(spacing);
}

// ----------------------------------------------------------------------


void
vtkHierarchicalGraphView::SetRadialLayout(bool value)
{
  this->TreeStrategy->SetRadial(value);
}

// ----------------------------------------------------------------------

void
vtkHierarchicalGraphView::SetRadialAngle(int angle)
{
  this->TreeStrategy->SetAngle(angle);
}

// ----------------------------------------------------------------------

void
vtkHierarchicalGraphView::SetBundlingStrength(double strength)
{
  this->HBundle->SetBundlingStrength(strength);
  this->SelectedGraphHBundle->SetBundlingStrength(strength);
}

// ----------------------------------------------------------------------

void
vtkHierarchicalGraphView::SetLogSpacingFactor(double value)
{
  this->TreeStrategy->SetLogSpacingValue(value);
}
