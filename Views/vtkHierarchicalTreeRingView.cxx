/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalTreeRingView.cxx
  
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

#include "vtkHierarchicalTreeRingView.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkEdgeCenters.h"
#include "vtkEdgeListIterator.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkGraphHierarchicalBundle.h"
#include "vtkGraphTransferDataToTree.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkInteractorStyleTreeRingHover.h"
#include "vtkKdTreeSelector.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarsToColors.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSplineFilter.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkTree.h"
#include "vtkTreeFieldAggregator.h"
#include "vtkVertexDegree.h"
#include "vtkViewTheme.h"
#include "vtkHardwareSelector.h"
#include "vtkTreeRingReversedLayoutStrategy.h"
#include "vtkTreeRingLayout.h"
#include "vtkTreeRingPointLayout.h"
#include "vtkTreeRingToPolyData.h"

#include <ctype.h> // for tolower()

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkHierarchicalTreeRingView, "1.9");
vtkStandardNewMacro(vtkHierarchicalTreeRingView);
//----------------------------------------------------------------------------
vtkHierarchicalTreeRingView::vtkHierarchicalTreeRingView()
{
  // Processing objects
  this->Coordinate             = vtkSmartPointer<vtkCoordinate>::New();
  this->VertexDegree           = vtkSmartPointer<vtkVertexDegree>::New();
  this->GraphVertexDegree      = vtkSmartPointer<vtkVertexDegree>::New();
  this->EdgeCenters            = vtkSmartPointer<vtkEdgeCenters>::New();
  this->TreeAggregation        = vtkSmartPointer<vtkTreeFieldAggregator>::New();
  this->EdgeLabelMapper        = vtkSmartPointer<vtkDynamic2DLabelMapper>::New();
  this->EdgeLabelActor         = vtkSmartPointer<vtkActor2D>::New();
  this->HBundle                = vtkSmartPointer<vtkGraphHierarchicalBundle>::New();
  this->Spline                 = vtkSmartPointer<vtkSplineFilter>::New();
  this->GraphEdgeMapper        = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->GraphEdgeActor         = vtkSmartPointer<vtkActor>::New();
  this->TreeVisibilityRepresentation = vtkSmartPointer<vtkDataRepresentation>::New();
  this->TransferAttributes     = vtkSmartPointer<vtkGraphTransferDataToTree>::New();

  //TreeRing objects
  this->TreeRingLayout         = vtkSmartPointer<vtkTreeRingLayout>::New();
  this->TreeRingLayoutStrategy = vtkSmartPointer<vtkTreeRingReversedLayoutStrategy>::New();
  this->TreeRingMapper         = vtkSmartPointer<vtkTreeRingToPolyData>::New();
  this->TreeRingMapper2        = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->TreeRingActor          = vtkSmartPointer<vtkActor>::New();
  this->TreeRingLabelMapper    = vtkSmartPointer<vtkDynamic2DLabelMapper>::New();
  this->TreeRingLabelActor     = vtkSmartPointer<vtkActor2D>::New();
  this->TreeRingPointLayout    = vtkSmartPointer<vtkTreeRingPointLayout>::New();
  
  // Selection objects
  this->HardwareSelector       = vtkSmartPointer<vtkHardwareSelector>::New();
  this->KdTreeSelector         = vtkSmartPointer<vtkKdTreeSelector>::New();
  this->ExtractSelectedGraph   = vtkSmartPointer<vtkExtractSelectedGraph>::New();
  this->SelectedGraphHBundle   = vtkSmartPointer<vtkGraphHierarchicalBundle>::New();
  this->SelectedGraphSpline    = vtkSmartPointer<vtkSplineFilter>::New();
  this->SelectedGraphMapper    = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->SelectedGraphActor     = vtkSmartPointer<vtkActor>::New();
  
  // Replace the interactor style.
//   vtkInteractorStyleRubberBand2D* style = vtkInteractorStyleRubberBand2D::New();
//   this->SetInteractorStyle(style);
//   style->Delete();
  vtkInteractorStyleTreeRingHover* style = vtkInteractorStyleTreeRingHover::New();
  style->SetLayout(this->TreeRingLayout);
  this->SetInteractorStyle(style);
  style->Delete();
  
  // Setup view
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
  this->InteractorStyle->AddObserver(vtkCommand::UserEvent, this->GetObserver());
//FIXME - jfsheph - this observer goes with rubber band selection
//  this->InteractorStyle->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());

  this->Coordinate->SetCoordinateSystemToDisplay();
  
  // Setup parameters on the various mappers and actors
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

  this->TransferAttributes->SetSourceArrayName("VertexDegree");
  this->TransferAttributes->SetTargetArrayName("GraphVertexDegree");  
  this->TransferAttributes->SetDefaultValue(1);

  this->TreeRingLabelMapper->SetLabelModeToLabelFieldData();
  this->TreeRingLabelMapper->GetLabelTextProperty()->SetColor(1,1,1);
  this->TreeRingLabelMapper->GetLabelTextProperty()->SetJustificationToCentered();
  this->TreeRingLabelMapper->GetLabelTextProperty()->SetVerticalJustificationToCentered();
  this->TreeRingLabelMapper->GetLabelTextProperty()->SetFontSize(12);
  this->TreeRingLabelMapper->GetLabelTextProperty()->SetItalic(0);
  this->TreeRingLabelMapper->GetLabelTextProperty()->SetLineOffset(-10);
//  this->TreeRingLabelMapper->SetPriorityArrayName("leaf_count");
  this->TreeRingLabelMapper->SetPriorityArrayName("GraphVertexDegree");
  this->TreeRingLabelActor->PickableOff();

  // Set default parameters
  this->SetVertexLabelArrayName("id");
  this->VertexLabelVisibilityOff();
  this->SetEdgeLabelArrayName("id");
  this->EdgeLabelVisibilityOff();
  this->ColorEdgesOff();

  // Misc variables
  this->BundlingStrength = .5;  
  this->InteriorLogSpacing = 1.;
  this->TreeRingPointLayout->SetLogSpacingValue( this->InteriorLogSpacing );

  // Apply default theme
  vtkViewTheme* theme = vtkViewTheme::New();
  this->ApplyViewTheme(theme);
  theme->Delete();

  // Make empty seleciton for default highlight
  this->EmptySelection = vtkSmartPointer<vtkSelection>::New();
  this->EmptySelection->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
  vtkSmartPointer<vtkIdTypeArray> arr = vtkSmartPointer<vtkIdTypeArray>::New();
  this->EmptySelection->SetSelectionList(arr);

  // Set filter attributes
  this->TreeAggregation->LeafVertexUnitSizeOn();
//  this->TreeAggregation->SetField("leaf_count");
  this->TreeAggregation->SetField("size");
  this->TreeRingLayout->SetLayoutStrategy(this->TreeRingLayoutStrategy);
  this->TreeRingLayoutStrategy->SetSizeFieldName("size");
  this->HBundle->SetBundlingStrength(this->BundlingStrength);
  this->SelectedGraphHBundle->SetBundlingStrength(this->BundlingStrength);
  this->Spline->SetMaximumNumberOfSubdivisions(16);
  
  // Connect pipeline:
  //
  // TreeRepresentation*
  //    |                                 
  // TreeAgg                              
  //    |                                 
  // VertexDegree 
  //    |                                 
  // TreeRingLayout
  //    |
  //    +--------------------- TreeRingToPolyData 
  //    |                          |      
  //    | GraphRepresentation**  TreeRingMapper
  //    |         |    +  |        |
  //    |         |    +  |      TreeRingActor 
  // TRPointLayout|    +  |
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
  this->VertexDegree->SetInputConnection(this->TreeAggregation->GetOutputPort());
  this->TransferAttributes->SetInputConnection(1, this->VertexDegree->GetOutputPort());
  this->TransferAttributes->SetInputConnection(0, this->GraphVertexDegree->GetOutputPort());
  
  this->TreeRingLayout->SetInputConnection(this->TransferAttributes->GetOutputPort());
  this->TreeRingPointLayout->SetInputConnection(this->TreeRingLayout->GetOutputPort());
  this->HBundle->SetInputConnection(1, this->TreeRingPointLayout->GetOutputPort(0));
  this->Spline->SetInputConnection(0, this->HBundle->GetOutputPort(0));
  this->EdgeCenters->SetInputConnection(this->TreeRingPointLayout->GetOutputPort());
  this->EdgeLabelMapper->SetInputConnection(this->EdgeCenters->GetOutputPort());
  this->EdgeLabelActor->SetMapper(this->EdgeLabelMapper);
  this->GraphEdgeMapper->SetInputConnection(this->Spline->GetOutputPort());
  this->GraphEdgeActor->SetMapper(this->GraphEdgeMapper);
  this->KdTreeSelector->SetInputConnection(this->TreeRingLayout->GetOutputPort());
  this->SelectedGraphHBundle->SetInputConnection(0, this->ExtractSelectedGraph->GetOutputPort());
  this->SelectedGraphHBundle->SetInputConnection(1, this->TreeRingPointLayout->GetOutputPort());
  this->SelectedGraphSpline->SetInputConnection(this->SelectedGraphHBundle->GetOutputPort());
  this->SelectedGraphMapper->SetInputConnection(this->SelectedGraphSpline->GetOutputPort());
  this->SelectedGraphActor->SetMapper(this->SelectedGraphMapper);
  this->SelectedGraphActor->GetProperty()->SetLineWidth(5.0);

//  this->TreeRingLayout->SetLayoutStrategy(0);
  this->TreeRingMapper->SetInputConnection(this->TreeRingLayout->GetOutputPort());
  this->TreeRingLayoutStrategy->SetRingThickness(1.);
  this->TreeRingPointLayout->SetExteriorRadius( this->TreeRingLayoutStrategy->GetInteriorRadius() );
  this->TreeRingMapper->SetShrinkPercentage(0.05);
  this->TreeRingLabelMapper->SetInputConnection(this->TreeRingLayout->GetOutputPort());
  this->TreeRingLabelActor->SetMapper(this->TreeRingLabelMapper);

  VTK_CREATE(vtkLookupTable, ColorLUT);
  ColorLUT->SetHueRange( 0.667, 0 );
  ColorLUT->Build();
  this->TreeRingMapper2->SetLookupTable(ColorLUT);
  this->TreeRingMapper2->SetInputConnection(this->TreeRingMapper->GetOutputPort());
  this->TreeRingActor->SetMapper(this->TreeRingMapper2);
  
  this->TreeVisibilityRepresentation->AddObserver(
    vtkCommand::SelectionChangedEvent, this->GetObserver());

  // Register any algorithm that can fire progress events with the superclass.
  this->RegisterProgress(this->TreeAggregation, "TreeAggregation");
  this->RegisterProgress(this->VertexDegree, "VertexDegree");
  this->RegisterProgress(this->TreeRingLayout, "TreeRingLayout");
  this->RegisterProgress(this->TreeRingPointLayout, "TreeRingPointLayout");
  this->RegisterProgress(this->HBundle, "HBundle");
  this->RegisterProgress(this->Spline, "Spline");
  this->RegisterProgress(this->GraphEdgeMapper, "CurvedEdgeMapper");
}

//----------------------------------------------------------------------------
vtkHierarchicalTreeRingView::~vtkHierarchicalTreeRingView()
{  
  // UnRegister any algorithm that can fire progress events from the superclass.
  this->UnRegisterProgress(this->TreeAggregation);
  this->UnRegisterProgress(this->VertexDegree);
  this->UnRegisterProgress(this->TreeRingLayout);
  this->UnRegisterProgress(this->TreeRingPointLayout);
  this->UnRegisterProgress(this->HBundle);
  this->UnRegisterProgress(this->Spline);
  this->UnRegisterProgress(this->GraphEdgeMapper);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalTreeRingView::SetHierarchyFromInputConnection(vtkAlgorithmOutput* conn)
{
  return this->SetRepresentationFromInputConnection(0, conn);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalTreeRingView::SetHierarchyFromInput(vtkDataObject* input)
{
  return this->SetRepresentationFromInput(0, input);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalTreeRingView::SetGraphFromInputConnection(vtkAlgorithmOutput* conn)
{
  return this->SetRepresentationFromInputConnection(1, conn);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkHierarchicalTreeRingView::SetGraphFromInput(vtkDataObject* input)
{
  return this->SetRepresentationFromInput(1, input);
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetVertexLabelArrayName(const char* name)
{
  this->TreeRingLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalTreeRingView::GetVertexLabelArrayName()
{
  return this->TreeRingLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetLabelPriorityArrayName(const char* name)
{
  this->TreeRingLabelMapper->SetPriorityArrayName(name);
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetEdgeLabelArrayName(const char* name)
{
  this->EdgeLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalTreeRingView::GetEdgeLabelArrayName()
{
  return this->EdgeLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetVertexLabelVisibility(bool vis)
{
  this->TreeRingLabelActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalTreeRingView::GetVertexLabelVisibility()
{
  return this->TreeRingLabelActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::VertexLabelVisibilityOn()
{
  this->TreeRingLabelActor->SetVisibility(true);
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::VertexLabelVisibilityOff()
{
  this->TreeRingLabelActor->SetVisibility(false);
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetEdgeLabelVisibility(bool vis)
{
  this->EdgeLabelActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalTreeRingView::GetEdgeLabelVisibility()
{
  return this->EdgeLabelActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::EdgeLabelVisibilityOn()
{
  this->EdgeLabelActor->SetVisibility(true);
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::EdgeLabelVisibilityOff()
{
  this->EdgeLabelActor->SetVisibility(false);
}

void vtkHierarchicalTreeRingView::SetRootAngles( double start, double end )
{
  this->TreeRingLayoutStrategy->SetRootStartAngle( start );
  this->TreeRingLayoutStrategy->SetRootEndAngle( end );
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetVertexColorArrayName(const char* name)
{
  this->TreeRingMapper2->SetScalarModeToUseCellFieldData();
  this->TreeRingMapper2->SelectColorArray(name); 
  
  double range[2]; 
  this->TreeRingMapper->Update();
  vtkDataArray *array = this->TreeRingMapper->GetOutput()->GetCellData()->GetArray(name);
  if(array)
  {
    array->GetRange(range);
    this->TreeRingMapper2->SetScalarRange( range[0], range[1] );
  }
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetEdgeColorArrayName(const char* name)
{   
  // Try to find the range the user-specified color array.
  double range[2];
  this->Spline->Update();
  cout << "stop1" << endl;
  vtkDataArray* arr = this->Spline->GetOutput()->GetCellData()->GetArray(name);
  cout << "stop2" << endl;
  if (arr)
    {
      cout << "stop5" << endl;
    this->GraphEdgeMapper->SetScalarModeToUseCellFieldData();
    this->GraphEdgeMapper->SelectColorArray(name); 
    arr->GetRange(range);  
    this->GraphEdgeMapper->SetScalarRange(range[0], range[1]);
    cout << "stop6" << endl;
    }
  else
    {
      cout << "stop3" << endl;
    vtkErrorMacro("Could not find array named: " << name);
    cout << "stop4" << endl;
    }
}

void vtkHierarchicalTreeRingView::SetEdgeColorToSplineFraction()
{
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
const char* vtkHierarchicalTreeRingView::GetEdgeColorArrayName()
{
  return this->GraphEdgeMapper->GetArrayName();
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetColorEdges(bool vis)
{
  this->GraphEdgeMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkHierarchicalTreeRingView::GetColorEdges()
{
  return this->GraphEdgeMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::ColorEdgesOn()
{
  this->GraphEdgeMapper->SetScalarVisibility(true);
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::ColorEdgesOff()
{
  this->GraphEdgeMapper->SetScalarVisibility(false);
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetupRenderWindow(vtkRenderWindow* win)
{
  this->Superclass::SetupRenderWindow(win);
  win->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
  this->Renderer->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::AddInputConnection( int port, int vtkNotUsed(index),
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
//    this->ExtractSelectedTree->SetInputConnection(1, selectionConn);
    haveTree = true;
    }
  else
    {
    this->HBundle->SetInputConnection(0, conn);
    this->GraphVertexDegree->SetInputConnection(0, conn);
    this->ExtractSelectedGraph->SetInputConnection(0, conn);
    this->ExtractSelectedGraph->SetInputConnection(1, selectionConn);
    haveGraph = true;
    }

  haveGraph = haveGraph || this->GetGraphRepresentation();
  haveTree = haveTree || this->GetTreeRepresentation();

  // If we have a graph and a tree, we are ready to go.
  if (haveGraph && haveTree)
  { 
    this->Renderer->AddActor( this->TreeRingActor );
    this->Renderer->AddActor( this->TreeRingLabelActor );
    this->Renderer->AddActor(this->SelectedGraphActor);
    this->Renderer->AddActor(this->EdgeLabelActor);
    this->Renderer->AddActor(this->GraphEdgeActor);
    
    this->Renderer->ResetCamera();
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::RemoveInputConnection( int port, int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* selectionConn)
{
  if( port == 0 )
  {
    if (this->TreeAggregation->GetNumberOfInputConnections(0) > 0 &&
        this->TreeAggregation->GetInputConnection(0, 0) == conn)
    {
      this->TreeAggregation->RemoveInputConnection(0, conn);
    }
  }
  else if( port == 1 )
  {
    if (this->HBundle->GetNumberOfInputConnections(0) > 0 &&
        this->HBundle->GetInputConnection(0, 0) == conn)
    {
      this->HBundle->RemoveInputConnection(0, conn);
      this->GraphVertexDegree->RemoveInputConnection(0, conn);
      this->ExtractSelectedGraph->RemoveInputConnection(0, conn);
      this->ExtractSelectedGraph->RemoveInputConnection(1, selectionConn);
    }
  }
  
  this->Renderer->RemoveActor(this->TreeRingActor);
  this->Renderer->RemoveActor(this->TreeRingLabelActor);
  this->Renderer->RemoveActor(this->SelectedGraphActor);
  this->Renderer->RemoveActor(this->EdgeLabelActor);
  this->Renderer->RemoveActor(this->GraphEdgeActor);
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::MapToXYPlane(
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
void vtkHierarchicalTreeRingView::ProcessEvents( vtkObject* caller, 
  unsigned long eventId, void* callData)
{
    // First make sure the view is set up.
  vtkDataRepresentation* treeRep = this->GetRepresentation();
  vtkDataRepresentation* graphRep = this->GetRepresentation(1, 0);
  
  if (!treeRep || !graphRep)
  {
    return;
  }

  if(caller == this->InteractorStyle && eventId == vtkCommand::UserEvent&& 
     this->TreeRingLayout->GetNumberOfInputConnections(0) > 0)
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
//FIXME-jfsheph - this is for rubber band selection...
//   else if (caller == this->InteractorStyle && eventId == vtkCommand::SelectionChangedEvent && 
//            this->TreeRingLayout->GetNumberOfInputConnections(0) > 0)
//     {
//     // Create the selection
//     unsigned int* rect = reinterpret_cast<unsigned int*>(callData);
//     bool singleSelectMode = false;
//     unsigned int pos1X = rect[0];
//     unsigned int pos1Y = rect[1];
//     unsigned int pos2X = rect[2];
//     unsigned int pos2Y = rect[3];
//     int stretch = 2;
//     if (pos1X == pos2X && pos1Y == pos2Y)
//       {
//       singleSelectMode = true;
//       pos1X = pos1X - stretch > 0 ? pos1X - stretch : 0;
//       pos1Y = pos1Y - stretch > 0 ? pos1Y - stretch : 0;
//       pos2X = pos2X + stretch;
//       pos2Y = pos2Y + stretch;
//       }
//     double pt1[2];
//     double pt2[2];
//     this->MapToXYPlane(pos1X, pos1Y, pt1[0], pt1[1]);
//     this->MapToXYPlane(pos2X, pos2Y, pt2[0], pt2[1]);
//     double minX = pt1[0] < pt2[0] ? pt1[0] : pt2[0];
//     double maxX = pt1[0] < pt2[0] ? pt2[0] : pt1[0];
//     double minY = pt1[1] < pt2[1] ? pt1[1] : pt2[1];
//     double maxY = pt1[1] < pt2[1] ? pt2[1] : pt1[1];
//     this->KdTreeSelector->SetSelectionBounds(
//       minX, maxX, minY, maxY, -1, 1);
//     this->KdTreeSelector->SetSingleSelection(singleSelectMode);
//     double radiusX = 2*(maxX-minX);
//     double radiusY = 2*(maxY-minY);
//     double dist2 = radiusX*radiusX + radiusY*radiusY;
//     this->KdTreeSelector->SetSingleSelectionThreshold(dist2);
//     this->KdTreeSelector->Update();
//     vtkSelection* kdSelection = this->KdTreeSelector->GetOutput();

//     // Convert to the proper selection type. Must be pedigree ids for this view.
//     this->TreeRingLayout->Update();
//     vtkSmartPointer<vtkSelection> vertexSelection;
//     vertexSelection.TakeReference(vtkConvertSelection::ToSelectionType(
//       kdSelection, this->TreeRingLayout->GetOutput(), vtkSelection::PEDIGREEIDS));

//     vtkSmartPointer<vtkSelection> selection = vtkSmartPointer<vtkSelection>::New();
//     selection->SetContentType(vtkSelection::SELECTIONS);

//     if (vertexSelection->GetSelectionList()->GetNumberOfTuples() > 0)
//       {
//       selection->AddChild(vertexSelection);
//       }
//     else
//       {
//       // If we didn't find any vertices, perform edge selection.
//       // The edge actor must be opaque for visible cell selection.
//       int scalarVis = this->GraphEdgeMapper->GetScalarVisibility();
//       this->GraphEdgeMapper->ScalarVisibilityOff();
//       vtkScalarsToColors* lookup = this->GraphEdgeMapper->GetLookupTable();
//       lookup->Register(0);
//       this->GraphEdgeMapper->SetLookupTable(0);
//       double opacity = this->GraphEdgeActor->GetProperty()->GetOpacity();
//       this->GraphEdgeActor->GetProperty()->SetOpacity(1.0);
      
//       unsigned int screenMinX = pos1X < pos2X ? pos1X : pos2X;
//       unsigned int screenMaxX = pos1X < pos2X ? pos2X : pos1X;
//       unsigned int screenMinY = pos1Y < pos2Y ? pos1Y : pos2Y;
//       unsigned int screenMaxY = pos1Y < pos2Y ? pos2Y : pos1Y;
//       this->HardwareSelector->SetRenderer(this->Renderer);
//       this->HardwareSelector->SetArea(screenMinX, screenMinY, screenMaxX, screenMaxY);
//       this->HardwareSelector->SetFieldAssociation(
//         vtkDataObject::FIELD_ASSOCIATION_CELLS);
//       vtkSmartPointer<vtkSelection> sel;
//       sel.TakeReference(this->HardwareSelector->Select());
//       vtkSmartPointer<vtkIdTypeArray> ids;
//       if (sel)
//         {
//         sel = sel->GetChild(0);
//         ids = sel? vtkIdTypeArray::SafeDownCast(sel->GetSelectionList()) : 0;
//         }

//       // Set the special edge actor back to normal.
//       this->GraphEdgeMapper->SetScalarVisibility(scalarVis);
//       this->GraphEdgeMapper->SetLookupTable(lookup);
//       lookup->Delete();
//       this->GraphEdgeActor->GetProperty()->SetOpacity(opacity);
      
//       vtkSmartPointer<vtkIdTypeArray> selectedIds = vtkSmartPointer<vtkIdTypeArray>::New();
//       for (vtkIdType i = 0; ids && (i < ids->GetNumberOfTuples()); i++)
//         {
//         vtkIdType edge = ids->GetValue(i);
//         selectedIds->InsertNextValue(edge);
//         if (singleSelectMode)
//           {
//           break;
//           }
//         }
      
//       // Start with polydata cell selection of lines.
//       vtkSmartPointer<vtkSelection> cellIndexSelection = vtkSmartPointer<vtkSelection>::New();
//       cellIndexSelection->SetContentType(vtkSelection::INDICES);
//       cellIndexSelection->SetFieldType(vtkSelection::CELL);
//       cellIndexSelection->SetSelectionList(selectedIds);

//       // Convert to pedigree ids.
//       this->HBundle->Update();
//       vtkSmartPointer<vtkSelection> edgeSelection;
//       edgeSelection.TakeReference(vtkConvertSelection::ToSelectionType(
//         cellIndexSelection, this->HBundle->GetOutput(), vtkSelection::PEDIGREEIDS));

//       // Make it an edge selection.
//       edgeSelection->SetFieldType(vtkSelection::EDGE);

//       if (edgeSelection->GetSelectionList()->GetNumberOfTuples() > 0)
//         {
//         selection->AddChild(edgeSelection);
//         }
//       }

//     // If this is a union selection, append the selection
//     if (rect[4] == vtkInteractorStyleRubberBand2D::SELECT_UNION)
//       {
//       vtkSelection* oldSelection =
//         this->GetRepresentation()->GetSelectionLink()->GetSelection();
//       selection->Union(oldSelection);
//       }
    
//     // Call select on the representation(s)
//     for (int i = 0; i < 2; ++i)
//       {
//       for (int j = 0; j < this->GetNumberOfRepresentations(i); ++j)
//         {
//         this->GetRepresentation(i, j)->Select(this, selection);
//         }
//       }
//     }
  else
    {
    Superclass::ProcessEvents(caller, eventId, callData);
    }
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::PrepareForRendering()
{
  if (!this->GetTreeRepresentation() || !this->GetGraphRepresentation())
    {
    return;
    }
  // Make sure the tree input connection is up to date.
  vtkDataRepresentation* treeRep = this->GetRepresentation();
  
  vtkAlgorithmOutput* treeConn = treeRep->GetInputConnection();
  if (this->TreeAggregation->GetInputConnection(0, 0) != treeConn)
    {
      this->RemoveInputConnection(0, 0,
                                  this->TreeAggregation->GetInputConnection(0, 0), NULL );
        //FIXME - jfsheph - what does setting this to NULL do?
//      this->ExtractSelectedTree->GetInputConnection(1, 0));
    this->AddInputConnection(0, 0, treeConn, treeRep->GetSelectionConnection());
    }

  // Make sure the graph input connection is up to date.
  vtkDataRepresentation* graphRep = this->GetRepresentation(1, 0);
  
  vtkAlgorithmOutput* graphConn = graphRep->GetInputConnection();
  if (this->HBundle->GetInputConnection(0, 0) != graphConn)
    {
      this->RemoveInputConnection(1, 0,
      this->HBundle->GetInputConnection(0, 0),
      this->ExtractSelectedGraph->GetInputConnection(1, 0));
      this->AddInputConnection(1, 0, graphConn, graphRep->GetSelectionConnection());
    }
  
  this->Superclass::PrepareForRendering();
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::ApplyViewTheme(vtkViewTheme* theme)
{
  // Take some parameters from the theme and apply
  // to objects within this class
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  this->Renderer->SetBackground2(theme->GetBackgroundColor2());
  this->Renderer->SetGradientBackground(true);
  
  this->EdgeLabelMapper->GetLabelTextProperty()->
    SetColor(theme->GetEdgeLabelColor());
  this->TreeRingLabelMapper->GetLabelTextProperty()->
    SetColor(theme->GetVertexLabelColor());
    
  // Pull selection info from theme, create a new theme, 
  // and pass to the selection graph mapper
  vtkViewTheme *selectTheme = vtkViewTheme::New();
  selectTheme->SetPointColor(theme->GetSelectedPointColor());
  selectTheme->SetCellColor(theme->GetSelectedCellColor());
  selectTheme->SetOutlineColor(theme->GetSelectedPointColor());  
  selectTheme->Delete();

  double color[3];
  theme->GetSelectedPointColor(color);
  vtkInteractorStyleTreeRingHover::SafeDownCast(this->InteractorStyle)->
    SetSelectionLightColor(color[0], color[1], color[2]);

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

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::SetHoverArrayName(const char* name)
{
  vtkInteractorStyleTreeRingHover::SafeDownCast(this->InteractorStyle)->SetLabelField(name);
}

//----------------------------------------------------------------------------
const char* vtkHierarchicalTreeRingView::GetHoverArrayName()
{
  return vtkInteractorStyleTreeRingHover::SafeDownCast(this->InteractorStyle)->GetLabelField();
}

void vtkHierarchicalTreeRingView::SetVertexLabelFontSize(const int size)
{
  this->TreeRingLabelMapper->GetLabelTextProperty()->SetFontSize(size);
}

int vtkHierarchicalTreeRingView::GetVertexLabelFontSize()
{
  return this->TreeRingLabelMapper->GetLabelTextProperty()->GetFontSize();
}

void vtkHierarchicalTreeRingView::SetEdgeLabelFontSize(const int size)
{
  this->EdgeLabelMapper->GetLabelTextProperty()->SetFontSize(size);
}

int vtkHierarchicalTreeRingView::GetEdgeLabelFontSize()
{
  return this->EdgeLabelMapper->GetLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkHierarchicalTreeRingView::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Coordinate: " << endl;
  this->Coordinate->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexDegree: " << endl;
  this->VertexDegree->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphVertexDegree: " << endl;
  this->GraphVertexDegree->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectedGraphMapper: " << endl;
  this->SelectedGraphMapper->PrintSelf(os, indent.GetNextIndent());
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
  os << indent << "TreeRingLabelMapper: " << endl;
  this->TreeRingLabelMapper->PrintSelf(os, indent.GetNextIndent());

  if (this->GetGraphRepresentation() && this->GetTreeRepresentation())
    {
    os << indent << "TreeRingLabelActor: " << endl;
    this->TreeRingLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeLabelActor: " << endl;
    this->EdgeLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "GraphActor: " << endl;
    this->GraphEdgeActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "TreeRingMapper: " << endl;
    this->TreeRingMapper->PrintSelf(os, indent.GetNextIndent());
    os << indent << "TreeRingActor: " << endl;
    this->TreeRingActor->PrintSelf(os, indent.GetNextIndent());
    }
}

// ----------------------------------------------------------------------
void
vtkHierarchicalTreeRingView::SetBundlingStrength(double strength)
{
  this->HBundle->SetBundlingStrength(strength);
  this->SelectedGraphHBundle->SetBundlingStrength(strength);
}

// ----------------------------------------------------------------------
void
vtkHierarchicalTreeRingView::SetInteriorLogSpacingFactor(double value)
{
  this->InteriorLogSpacing = value;
  this->TreeRingPointLayout->SetLogSpacingValue(value);
}

// ----------------------------------------------------------------------
void
vtkHierarchicalTreeRingView::SetSectorShrinkFactor(double value)
{
  this->TreeRingMapper->SetShrinkPercentage(value);
}
