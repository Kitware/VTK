/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeAreaView.cxx

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

#include "vtkTreeAreaView.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkAreaLayout.h"
#include "vtkAreaLayoutStrategy.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataRepresentation.h"
#include "vtkDynamic2DLabelMapper.h"
#include "vtkEdgeCenters.h"
#include "vtkEdgeListIterator.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkExtractSelectedPolyDataIds.h"
#include "vtkGraphHierarchicalBundle.h"
#include "vtkTransferAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInteractorStyleRubberBand2D.h"
#include "vtkInteractorStyleAreaSelectHover.h"
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
#include "vtkSelectionNode.h"
#include "vtkSplineFilter.h"
#include "vtkStackedTreeLayoutStrategy.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"
#include "vtkTree.h"
#include "vtkTreeFieldAggregator.h"
#include "vtkTreeLevelsFilter.h"
#include "vtkTreeMapToPolyData.h"
#include "vtkTreeRingToPolyData.h"
#include "vtkVertexDegree.h"
#include "vtkViewTheme.h"
#include "vtkHardwareSelector.h"

#define VTK_CREATE(type, name)                                  \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkCxxRevisionMacro(vtkTreeAreaView, "1.3");
vtkStandardNewMacro(vtkTreeAreaView);
//----------------------------------------------------------------------------
vtkTreeAreaView::vtkTreeAreaView()
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
  this->TransferAttributes     = vtkSmartPointer<vtkTransferAttributes>::New();
  this->TreeLevels             = vtkSmartPointer<vtkTreeLevelsFilter>::New();

  // Area objects
  this->AreaLayout         = vtkSmartPointer<vtkAreaLayout>::New();
  this->AreaToPolyData     = vtkTreeRingToPolyData::New();
  this->AreaMapper         = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->AreaActor          = vtkSmartPointer<vtkActor>::New();
  this->AreaLabelMapper    = 0;
  this->AreaLabelActor     = vtkSmartPointer<vtkActor2D>::New();

  // Graph selection objects
  this->HardwareSelector       = vtkSmartPointer<vtkHardwareSelector>::New();
  this->KdTreeSelector         = vtkSmartPointer<vtkKdTreeSelector>::New();
  this->ExtractSelectedGraph   = vtkSmartPointer<vtkExtractSelectedGraph>::New();
  this->SelectedGraphHBundle   = vtkSmartPointer<vtkGraphHierarchicalBundle>::New();
  this->SelectedGraphSpline    = vtkSmartPointer<vtkSplineFilter>::New();
  this->SelectedGraphMapper    = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->SelectedGraphActor     = vtkSmartPointer<vtkActor>::New();

  // Area selection objects
  this->ConvertSelection       = vtkSmartPointer<vtkConvertSelection>::New();
  this->ExtractSelectedAreas   = vtkSmartPointer<vtkExtractSelectedPolyDataIds>::New();
  this->SelectedAreaMapper     = vtkSmartPointer<vtkPolyDataMapper>::New();
  this->SelectedAreaActor      = vtkSmartPointer<vtkActor>::New();

  this->SetAreaColorArrayName("color");

  // Replace the interactor style.
  vtkInteractorStyleAreaSelectHover* style = vtkInteractorStyleAreaSelectHover::New();
  style->SetLayout(this->AreaLayout);
  this->SetInteractorStyle(style);
  style->Delete();

  // Setup view
  this->Renderer->GetActiveCamera()->ParallelProjectionOn();
//  this->InteractorStyle->AddObserver(vtkCommand::UserEvent, this->GetObserver());
//FIXME - jfsheph - this observer goes with rubber band selection
  this->InteractorStyle->AddObserver(vtkCommand::SelectionChangedEvent, this->GetObserver());

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
  this->SelectedGraphActor->SetPosition(0, 0, 0.8);
  this->SelectedGraphMapper->SetScalarVisibility(false);

  this->TransferAttributes->SetSourceArrayName("VertexDegree");
  this->TransferAttributes->SetTargetArrayName("GraphVertexDegree");
  this->TransferAttributes->SetSourceFieldType(vtkDataObject::FIELD_ASSOCIATION_VERTICES);
  this->TransferAttributes->SetTargetFieldType(vtkDataObject::FIELD_ASSOCIATION_VERTICES);
  this->TransferAttributes->SetDefaultValue(1);

  vtkDynamic2DLabelMapper* areaMapper = vtkDynamic2DLabelMapper::New();
  this->SetAreaLabelMapper(areaMapper);
  areaMapper->Delete();
  this->AreaLabelActor->PickableOff();

  VTK_CREATE(vtkStackedTreeLayoutStrategy, strategy);
  strategy->SetReverse(true);
  this->AreaLayout->SetLayoutStrategy(strategy);
  this->AreaLayout->SetAreaArrayName("area");
  this->SetShrinkPercentage(0.1);
  this->AreaToPolyData->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "area");

  // Set default parameters
  this->SetAreaLabelArrayName("id");
  this->AreaLabelVisibilityOff();
  this->SetEdgeLabelArrayName("id");
  this->EdgeLabelVisibilityOff();
  this->ColorEdgesOff();

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
  this->SetBundlingStrength(0.5);
  this->Spline->SetMaximumNumberOfSubdivisions(16);

  // Connect pipeline:
  //
  // TreeRepresentation*
  //    |                        GraphRepresentation**
  // TreeLevels                   |    |   .  |
  //    |                         |    |   .  |
  // VertexDegree  GraphVertexDegree   |   .  |
  //    |    |      |                  |   .  |
  //    |  TransferAttributes          |   .  |
  //    |           |                  |   .  |
  //   OR***--------+                  |   .  |
  //    |                              |   .  |
  // TreeAggregation                   |   .  |
  //    |                              |   .  |
  // AreaLayout                        |   .  |
  //    |                              |   .  |
  //    +-------- AreaToPolyData       |   .  |
  //    |            |   |             |   .  |
  //    |            | AreaMapper      |   .  |
  //    |            |   |             |   .  |
  //    |            | AreaActor       |   .  |
  //    |            |                 |   .  |
  //    |            +--    TreeRep    |   .  |
  //    |            |  \     .        |   .  |
  //    |            |ConvertSelection |   .  |
  //    |            |        .        |   .  |
  //    |         ExtractSelectedAreas |   .  |
  //    |                |             |   .  |
  //    |         SelectedAreaMapper   |   .  |
  //    |                |             |   .  |
  //    |         SelectedAreaActor    |   .  |
  //    +-----+                        |   .  |
  //          |   +--------------------+   .  |
  //          |   |                        .  |
  //          |   |              ExtractSelectedGraph
  //          |   |                      |
  //         HBundle             SelectedGraphHBundle
  //            |                        |
  //         Spline              SelectedGraphSpline
  //            |                        |
  //         GraphMapper         SelectedGraphMapper
  //            |                        |
  //         GraphActor          SelectedGraphActor
  //
  // *   - The TreeRepresentation is retrieved with GetRepresentation(0,0)
  // **  - The GraphRepresentation is retrieved with GetRepresentation(1,0)
  // *** - If there is a graph representation, transfers vertex degree from
  //       the graph to the tree.
  // .   - Selection connection
  // -   - Data connection
  //
  this->VertexDegree->SetInputConnection(this->TreeLevels->GetOutputPort());
  this->TransferAttributes->SetInputConnection(0, this->VertexDegree->GetOutputPort());
  this->TransferAttributes->SetInputConnection(1, this->GraphVertexDegree->GetOutputPort());

  this->TreeAggregation->SetInputConnection(this->VertexDegree->GetOutputPort());
  this->AreaLayout->SetInputConnection(this->TreeAggregation->GetOutputPort());
  this->HBundle->SetInputConnection(1, this->AreaLayout->GetOutputPort(1));
  this->Spline->SetInputConnection(0, this->HBundle->GetOutputPort(0));
  this->EdgeCenters->SetInputConnection(this->AreaLayout->GetOutputPort(1));
  this->EdgeLabelMapper->SetInputConnection(this->EdgeCenters->GetOutputPort());
  this->EdgeLabelActor->SetMapper(this->EdgeLabelMapper);
  this->GraphEdgeMapper->SetInputConnection(this->Spline->GetOutputPort());
  this->GraphEdgeActor->SetMapper(this->GraphEdgeMapper);
  this->KdTreeSelector->SetInputConnection(this->AreaLayout->GetOutputPort());
  this->ExtractSelectedGraph->SetInput(1, this->EmptySelection);
  this->SelectedGraphHBundle->SetInputConnection(0, this->ExtractSelectedGraph->GetOutputPort());
  this->SelectedGraphHBundle->SetInputConnection(1, this->AreaLayout->GetOutputPort(1));
  this->SelectedGraphSpline->SetInputConnection(this->SelectedGraphHBundle->GetOutputPort());
  this->SelectedGraphMapper->SetInputConnection(this->SelectedGraphSpline->GetOutputPort());
  this->SelectedGraphActor->SetMapper(this->SelectedGraphMapper);
  this->SelectedGraphActor->GetProperty()->SetLineWidth(5.0);

  this->AreaToPolyData->SetInputConnection(this->AreaLayout->GetOutputPort());
  this->AreaLabelMapper->SetInputConnection(this->AreaLayout->GetOutputPort());
  this->AreaLabelActor->SetMapper(this->AreaLabelMapper);

  this->ConvertSelection->SetInput(0, this->EmptySelection);
  this->ConvertSelection->SetInputConnection(1, this->AreaToPolyData->GetOutputPort());
  this->ExtractSelectedAreas->SetInputConnection(0, this->AreaToPolyData->GetOutputPort());
  this->ExtractSelectedAreas->SetInputConnection(1, this->ConvertSelection->GetOutputPort());
  this->SelectedAreaMapper->SetInputConnection(this->ExtractSelectedAreas->GetOutputPort());
  this->SelectedAreaActor->SetMapper(this->SelectedAreaMapper);

  this->ConvertSelection->SetOutputType(vtkSelectionNode::INDICES);
  this->ConvertSelection->SetInputFieldType(vtkSelectionNode::CELL);
  this->SelectedAreaMapper->ScalarVisibilityOff();
  this->SelectedAreaActor->SetPosition(0.0, 0.0, 0.0005);

  VTK_CREATE(vtkLookupTable, ColorLUT);
  ColorLUT->SetHueRange( 0.667, 0 );
  ColorLUT->Build();
  this->AreaMapper->SetLookupTable(ColorLUT);
  this->AreaMapper->SetInputConnection(this->AreaToPolyData->GetOutputPort());
  this->AreaActor->SetMapper(this->AreaMapper);
  this->GraphEdgeActor->SetPosition(0.0, 0.0, 1.0);

  // Register any algorithm that can fire progress events with the superclass.
  this->RegisterProgress(this->TreeAggregation, "TreeAggregation");
  this->RegisterProgress(this->VertexDegree, "VertexDegree");
  this->RegisterProgress(this->AreaLayout, "AreaLayout");
  this->RegisterProgress(this->HBundle, "HBundle");
  this->RegisterProgress(this->Spline, "Spline");
  this->RegisterProgress(this->GraphEdgeMapper, "CurvedEdgeMapper");
}

//----------------------------------------------------------------------------
vtkTreeAreaView::~vtkTreeAreaView()
{
  if (this->AreaToPolyData)
    {
    this->AreaToPolyData->Delete();
    this->AreaToPolyData = 0;
    }
  if (this->AreaLabelMapper)
    {
    this->AreaLabelMapper->Delete();
    this->AreaLabelMapper = 0;
    }
  // UnRegister any algorithm that can fire progress events from the superclass.
  this->UnRegisterProgress(this->TreeAggregation);
  this->UnRegisterProgress(this->VertexDegree);
  this->UnRegisterProgress(this->AreaLayout);
  this->UnRegisterProgress(this->HBundle);
  this->UnRegisterProgress(this->Spline);
  this->UnRegisterProgress(this->GraphEdgeMapper);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkTreeAreaView::SetTreeFromInputConnection(vtkAlgorithmOutput* conn)
{
  return this->SetRepresentationFromInputConnection(0, conn);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkTreeAreaView::SetTreeFromInput(vtkTree* input)
{
  return this->SetRepresentationFromInput(0, input);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkTreeAreaView::SetGraphFromInputConnection(vtkAlgorithmOutput* conn)
{
  return this->SetRepresentationFromInputConnection(1, conn);
}

//----------------------------------------------------------------------------
vtkDataRepresentation* vtkTreeAreaView::SetGraphFromInput(vtkGraph* input)
{
  return this->SetRepresentationFromInput(1, input);
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaLabelArrayName(const char* name)
{
  this->AreaLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetAreaLabelArrayName()
{
  return this->AreaLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaSizeArrayName(const char* name)
{
  this->TreeAggregation->SetField(name);
  this->TreeAggregation->LeafVertexUnitSizeOff();
  this->AreaLayout->SetSizeArrayName(name);
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetLabelPriorityArrayName(const char* name)
{
  vtkDynamic2DLabelMapper* dynamic =
    vtkDynamic2DLabelMapper::SafeDownCast(this->AreaLabelMapper);
  if (dynamic)
    {
    dynamic->SetPriorityArrayName(name);
    }
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeLabelArrayName(const char* name)
{
  this->EdgeLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetEdgeLabelArrayName()
{
  return this->EdgeLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaLabelVisibility(bool vis)
{
  this->AreaLabelActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetAreaLabelVisibility()
{
  return this->AreaLabelActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::AreaLabelVisibilityOn()
{
  this->AreaLabelActor->SetVisibility(true);
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::AreaLabelVisibilityOff()
{
  this->AreaLabelActor->SetVisibility(false);
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeLabelVisibility(bool vis)
{
  this->EdgeLabelActor->SetVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetEdgeLabelVisibility()
{
  return this->EdgeLabelActor->GetVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::EdgeLabelVisibilityOn()
{
  this->EdgeLabelActor->SetVisibility(true);
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::EdgeLabelVisibilityOff()
{
  this->EdgeLabelActor->SetVisibility(false);
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaColorArrayName(const char* name)
{
  this->AreaMapper->SetScalarModeToUseCellFieldData();
  this->AreaMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeColorArrayName(const char* name)
{
  this->GraphEdgeMapper->SetScalarModeToUseCellFieldData();
  this->GraphEdgeMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeColorToSplineFraction()
{
  this->GraphEdgeMapper->SetScalarModeToUsePointFieldData();
  this->GraphEdgeMapper->SelectColorArray("fraction");
  if (this->GetGraphRepresentation())
    {
    // Try to find the range the fraction color array.
    double range[2];
    vtkDataArray* arr = 0;
    arr = this->Spline->GetOutput()->GetPointData()->GetArray("fraction");
    if (arr)
      {
      arr->GetRange(range);
      this->GraphEdgeMapper->SetScalarRange(range[0], range[1]);
      }
    }
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetEdgeColorArrayName()
{
  return this->GraphEdgeMapper->GetArrayName();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetColorEdges(bool vis)
{
  this->GraphEdgeMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetColorEdges()
{
  return this->GraphEdgeMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetColorVertices(bool vis)
{
  this->AreaMapper->SetScalarVisibility(vis);
}

//----------------------------------------------------------------------------
bool vtkTreeAreaView::GetColorVertices()
{
  return this->AreaMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetShrinkPercentage(double pcent)
{
  this->AreaLayout->GetLayoutStrategy()->SetShrinkPercentage(pcent);
}

//----------------------------------------------------------------------------
double vtkTreeAreaView::GetShrinkPercentage()
{
  return this->AreaLayout->GetLayoutStrategy()->GetShrinkPercentage();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetupRenderWindow(vtkRenderWindow* win)
{
  this->Superclass::SetupRenderWindow(win);
  win->GetInteractor()->SetInteractorStyle(this->InteractorStyle);
  this->Renderer->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::AddInputConnection(
    int port, int vtkNotUsed(index),
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn)
{
  bool haveGraph = false;
  bool haveTree = false;

  // Port 0 is designated as the tree and port 1 is the graph
  if( port == 0 )
    {
    this->TreeLevels->SetInputConnection(0, conn);
    if (selectionConn)
      {
      this->ConvertSelection->SetInputConnection(0, selectionConn);
      }
    else
      {
      this->ConvertSelection->SetInput(0, this->EmptySelection);
      }
    haveTree = true;
    }
  else
    {
    this->HBundle->SetInputConnection(0, conn);
    this->GraphVertexDegree->SetInputConnection(0, conn);
    this->ExtractSelectedGraph->SetInputConnection(0, conn);
    if (selectionConn)
      {
      this->ExtractSelectedGraph->SetInputConnection(1, selectionConn);
      }
    else
      {
      this->ExtractSelectedGraph->SetInput(1, this->EmptySelection);
      }
    this->TreeAggregation->SetInputConnection(this->TransferAttributes->GetOutputPort());
    haveGraph = true;
    }

  haveGraph = haveGraph || this->GetGraphRepresentation();
  haveTree = haveTree || this->GetTreeRepresentation();

  // If we have a tree, we are ready to go.
  if (haveTree)
    {
    this->Renderer->AddActor( this->AreaActor );
    this->Renderer->AddActor( this->AreaLabelActor );
    this->Renderer->AddActor( this->SelectedAreaActor );
    if (haveGraph)
      {
      this->Renderer->AddActor(this->SelectedGraphActor);
      this->Renderer->AddActor(this->EdgeLabelActor);
      this->Renderer->AddActor(this->GraphEdgeActor);
      }
    this->Renderer->ResetCamera();
    }
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::RemoveInputConnection( int port, int vtkNotUsed(index),
  vtkAlgorithmOutput* conn, vtkAlgorithmOutput* selectionConn)
{
  if( port == 0 )
    {
    if (this->TreeLevels->GetNumberOfInputConnections(0) > 0 &&
        this->TreeLevels->GetInputConnection(0, 0) == conn)
      {
      this->TreeLevels->RemoveInputConnection(0, conn);
      }
    this->Renderer->RemoveActor(this->AreaActor);
    this->Renderer->RemoveActor(this->SelectedAreaActor);
    this->Renderer->RemoveActor(this->AreaLabelActor);
    this->Renderer->RemoveActor(this->SelectedGraphActor);
    this->Renderer->RemoveActor(this->EdgeLabelActor);
    this->Renderer->RemoveActor(this->GraphEdgeActor);
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
      this->TreeAggregation->SetInputConnection(this->VertexDegree->GetOutputPort());
      }
    this->Renderer->RemoveActor(this->SelectedGraphActor);
    this->Renderer->RemoveActor(this->EdgeLabelActor);
    this->Renderer->RemoveActor(this->GraphEdgeActor);
    }
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::MapToXYPlane(
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
void vtkTreeAreaView::ProcessEvents( vtkObject* caller,
  unsigned long eventId, void* callData)
{
    // First make sure the view is set up.
  vtkDataRepresentation* treeRep = this->GetTreeRepresentation();
  vtkDataRepresentation* graphRep = this->GetGraphRepresentation();

  if (!treeRep)
    {
    return;
    }

 //This is for rubber band selection...
 if (caller == this->InteractorStyle &&
     eventId == vtkCommand::SelectionChangedEvent &&
     this->TreeAggregation->GetNumberOfInputConnections(0) > 0)
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

   // The ring and edge actor must be opaque for visible cell selection.
   this->SelectedAreaActor->VisibilityOff();
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
   vtkSmartPointer<vtkIdTypeArray> edgeIds;
   vtkSmartPointer<vtkIdTypeArray> vertexIds;
   if (sel)
     {
     for (unsigned int i = 0; i < sel->GetNumberOfNodes(); ++i)
       {
       vtkSelectionNode* node = sel->GetNode(i);
       vtkObjectBase* prop = node->GetProperties()->Get(vtkSelectionNode::PROP());
       if (prop == this->GraphEdgeActor)
         {
         edgeIds = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
         }
       else if (prop == this->AreaActor)
         {
         vertexIds = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
         }
       }
     }

   // Set the ring and edge actors back to normal.
   this->SelectedAreaActor->VisibilityOn();
   this->GraphEdgeMapper->SetScalarVisibility(scalarVis);
   this->GraphEdgeMapper->SetLookupTable(lookup);
   lookup->Delete();
   this->GraphEdgeActor->GetProperty()->SetOpacity(opacity);

   // If we are in single select mode, make sure to select only the vertex
   // that is being hovered over.
   if (singleSelectMode)
     {
     vtkInteractorStyleAreaSelectHover* style =
       vtkInteractorStyleAreaSelectHover::SafeDownCast(this->InteractorStyle);
     vtkIdType v = style->GetIdAtPos(rect[0], rect[1]);
     vertexIds = vtkSmartPointer<vtkIdTypeArray>::New();
     if (v >= 0)
       {
       vertexIds->InsertNextValue(v);
       }
     }

   vtkSmartPointer<vtkIdTypeArray> selectedIds = vtkSmartPointer<vtkIdTypeArray>::New();
   for (vtkIdType i = 0; edgeIds && (i < edgeIds->GetNumberOfTuples()); i++)
     {
     vtkIdType edge = edgeIds->GetValue(i);
     selectedIds->InsertNextValue(edge);
     if (singleSelectMode)
       {
       break;
       }
     }

   if (graphRep)
     {
     // Start with polydata cell selection of lines.
     vtkSmartPointer<vtkSelection> cellIndexSelection = vtkSmartPointer<vtkSelection>::New();
     vtkSmartPointer<vtkSelectionNode> cellIndexNode = vtkSmartPointer<vtkSelectionNode>::New();
     cellIndexNode->SetContentType(vtkSelectionNode::INDICES);
     cellIndexNode->SetFieldType(vtkSelectionNode::CELL);
     cellIndexNode->SetSelectionList(selectedIds);
     cellIndexSelection->AddNode(cellIndexNode);

     // Convert to pedigree ids.
     // Make it an edge selection.
     this->HBundle->Update();
     vtkSmartPointer<vtkSelection> edgeSelection;
     edgeSelection.TakeReference(vtkConvertSelection::ToSelectionType(
           cellIndexSelection, this->HBundle->GetOutput(), vtkSelectionNode::PEDIGREEIDS));
     edgeSelection->GetNode(0)->SetFieldType(vtkSelectionNode::EDGE);
     if (edgeSelection->GetNode(0)->GetSelectionList()->GetNumberOfTuples() == 0)
       {
       edgeSelection->RemoveAllNodes();
       }

     // If this is a union selection, append the selection
     if (rect[4] == vtkInteractorStyleRubberBand2D::SELECT_UNION)
       {
       vtkSelection* oldEdgeSelection =
         graphRep->GetSelectionLink()->GetSelection();
       edgeSelection->Union(oldEdgeSelection);
       }
     graphRep->Select(this, edgeSelection);
     }

   // Now find selected vertices.
   vtkSmartPointer<vtkIdTypeArray> selectedVertexIds = vtkSmartPointer<vtkIdTypeArray>::New();
   for (vtkIdType i = 0; vertexIds && (i < vertexIds->GetNumberOfTuples()); i++)
     {
     vtkIdType v = vertexIds->GetValue(i);
     selectedVertexIds->InsertNextValue(v);
     if (singleSelectMode)
       {
       break;
       }
     }

   // Create a vertex selection.
   vtkSmartPointer<vtkSelection> vertexIndexSelection = vtkSmartPointer<vtkSelection>::New();
   vtkSmartPointer<vtkSelectionNode> vertexIndexNode = vtkSmartPointer<vtkSelectionNode>::New();
   vertexIndexNode->SetContentType(vtkSelectionNode::INDICES);
   vertexIndexNode->SetFieldType(vtkSelectionNode::CELL);
   vertexIndexNode->SetSelectionList(selectedVertexIds);
   vertexIndexSelection->AddNode(vertexIndexNode);

   // Convert to pedigree ids.
   // Make it a vertex selection.
   this->AreaToPolyData->Update();
   vtkSmartPointer<vtkSelection> vertexSelection;
   vertexSelection.TakeReference(vtkConvertSelection::ToSelectionType(
         vertexIndexSelection, this->AreaToPolyData->GetOutput(), vtkSelectionNode::PEDIGREEIDS));
   vertexSelection->GetNode(0)->SetFieldType(vtkSelectionNode::VERTEX);
   if (vertexSelection->GetNode(0)->GetSelectionList()->GetNumberOfTuples() == 0)
     {
     vertexSelection->RemoveAllNodes();
     }

   // If this is a union selection, append the selection
   if (rect[4] == vtkInteractorStyleRubberBand2D::SELECT_UNION)
     {
     vtkSelection* oldVertexSelection =
       treeRep->GetSelectionLink()->GetSelection();
     vertexSelection->Union(oldVertexSelection);
     }

   treeRep->Select(this, vertexSelection);
   }
 else
   {
   Superclass::ProcessEvents(caller, eventId, callData);
   }
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::PrepareForRendering()
{
  vtkDataRepresentation* treeRep = this->GetTreeRepresentation();

  if (treeRep)
    {
    // Make sure the tree input connection is up to date.
    vtkAlgorithmOutput* treeConn = treeRep->GetInputConnection();
    vtkAlgorithmOutput* selectionConn = treeRep->GetSelectionConnection();
    if (this->TreeLevels->GetInputConnection(0, 0) != treeConn ||
        this->ConvertSelection->GetInputConnection(0, 0) != selectionConn)
      {
      this->AddInputConnection(0, 0, treeConn, selectionConn);
      }

    // Make sure vertex color range is up-to-date.
    if (this->GetColorVertices())
      {
      double range[2];
      this->AreaToPolyData->Update();
      vtkDataArray *array =
        this->AreaToPolyData->GetOutput()->GetCellData()->GetArray(this->AreaMapper->GetArrayName());
      if(array)
        {
        array->GetRange(range);
        this->AreaMapper->SetScalarRange(range[0], range[1]);
        }
      }
    }

  vtkDataRepresentation* graphRep = this->GetGraphRepresentation();

  if (graphRep)
    {
    // Make sure the graph input connection is up to date.
    vtkAlgorithmOutput* graphConn = graphRep->GetInputConnection();
    vtkAlgorithmOutput* selectionConn = graphRep->GetSelectionConnection();
    if (this->HBundle->GetInputConnection(0, 0) != graphConn ||
        this->ExtractSelectedGraph->GetInputConnection(1, 0) != selectionConn)
      {
      this->AddInputConnection(1, 0, graphConn, selectionConn);
      }

    // Make sure edge color range is up-to-date.
    if (this->GetColorEdges())
      {
      double range[2];
      this->HBundle->Update();
      vtkDataArray *array = this->HBundle->GetOutput()->GetCellData()->
        GetArray(this->GraphEdgeMapper->GetArrayName());
      if(array)
        {
        array->GetRange(range);
        this->GraphEdgeMapper->SetScalarRange(range[0], range[1]);
        }
      }
    }

  this->Superclass::PrepareForRendering();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::ApplyViewTheme(vtkViewTheme* theme)
{
  // Take some parameters from the theme and apply
  // to objects within this class
  this->Renderer->SetBackground(theme->GetBackgroundColor());
  this->Renderer->SetBackground2(theme->GetBackgroundColor2());
  this->Renderer->SetGradientBackground(true);

  this->EdgeLabelMapper->GetLabelTextProperty()->
    SetColor(theme->GetEdgeLabelColor());
  this->AreaLabelMapper->GetLabelTextProperty()->
    SetColor(theme->GetVertexLabelColor());

  // Pull selection info from theme, create a new theme,
  // and pass to the selection graph mapper
  vtkViewTheme *selectTheme = vtkViewTheme::New();
  selectTheme->SetPointColor(theme->GetSelectedPointColor());
  selectTheme->SetCellColor(theme->GetSelectedCellColor());
  selectTheme->SetOutlineColor(theme->GetSelectedPointColor());
  selectTheme->Delete();

//   double color[3];
//   theme->GetSelectedPointColor(color);
//   vtkInteractorStyleAreaSelectHover::SafeDownCast(this->InteractorStyle)->
//     SetSelectionLightColor(color[0], color[1], color[2]);

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
void vtkTreeAreaView::SetAreaHoverArrayName(const char* name)
{
  vtkInteractorStyleAreaSelectHover::SafeDownCast(this->InteractorStyle)->SetLabelField(name);
}

//----------------------------------------------------------------------------
const char* vtkTreeAreaView::GetAreaHoverArrayName()
{
  return vtkInteractorStyleAreaSelectHover::SafeDownCast(this->InteractorStyle)->GetLabelField();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetAreaLabelFontSize(const int size)
{
  this->AreaLabelMapper->GetLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int vtkTreeAreaView::GetAreaLabelFontSize()
{
  return this->AreaLabelMapper->GetLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetEdgeLabelFontSize(const int size)
{
  this->EdgeLabelMapper->GetLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::SetLayoutStrategy(vtkAreaLayoutStrategy* strategy)
{
  if (strategy)
    {
    this->AreaLayout->SetLayoutStrategy(strategy);
    }
  else
    {
    vtkErrorMacro("Area layout strategy must be non-null.");
    }
}

//----------------------------------------------------------------------------
vtkAreaLayoutStrategy* vtkTreeAreaView::GetLayoutStrategy()
{
  return this->AreaLayout->GetLayoutStrategy();
}

//----------------------------------------------------------------------------
int vtkTreeAreaView::GetEdgeLabelFontSize()
{
  return this->EdgeLabelMapper->GetLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkTreeAreaView::PrintSelf(ostream& os, vtkIndent indent)
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
  os << indent << "AreaLabelMapper: " << endl;
  this->AreaLabelMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "AreaToPolyData: " << endl;
  this->AreaToPolyData->PrintSelf(os, indent.GetNextIndent());

  if (this->GetGraphRepresentation() && this->GetTreeRepresentation())
    {
    os << indent << "AreaLabelActor: " << endl;
    this->AreaLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeLabelActor: " << endl;
    this->EdgeLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "GraphActor: " << endl;
    this->GraphEdgeActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "AreaMapper: " << endl;
    this->AreaMapper->PrintSelf(os, indent.GetNextIndent());
    os << indent << "AreaActor: " << endl;
    this->AreaActor->PrintSelf(os, indent.GetNextIndent());
    }
}

// ----------------------------------------------------------------------
void vtkTreeAreaView::SetBundlingStrength(double strength)
{
  this->HBundle->SetBundlingStrength(strength);
  this->SelectedGraphHBundle->SetBundlingStrength(strength);
}

// ----------------------------------------------------------------------
void vtkTreeAreaView::SetAreaToPolyData(vtkPolyDataAlgorithm* poly)
{
  if (poly && this->AreaToPolyData != poly)
    {
    this->Modified();
    poly->SetInputConnection(this->AreaLayout->GetOutputPort());
    this->AreaMapper->SetInputConnection(poly->GetOutputPort());
    this->ConvertSelection->SetInputConnection(1, poly->GetOutputPort());
    this->ExtractSelectedAreas->SetInputConnection(0, poly->GetOutputPort());
    poly->SetInputArrayToProcess(
        0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "area");
    poly->Register(this);
    vtkPolyDataAlgorithm* tmpPoly = this->AreaToPolyData;
    this->AreaToPolyData = poly;
    if (tmpPoly)
      {
      tmpPoly->Delete();
      }
    }
  else
    {
    vtkErrorMacro("Area to polydata must not be null.");
    }
}

// ----------------------------------------------------------------------
void vtkTreeAreaView::SetAreaLabelMapper(vtkLabeledDataMapper* mapper)
{
  if (mapper && this->AreaLabelMapper != mapper)
    {
    this->Modified();
    mapper->SetInputConnection(this->AreaLayout->GetOutputPort());
    this->AreaLabelActor->SetMapper(mapper);
    mapper->Register(this);
    vtkLabeledDataMapper* tmpMapper = this->AreaLabelMapper;
    this->AreaLabelMapper = mapper;
    this->AreaLabelMapper->SetLabelModeToLabelFieldData();
    this->AreaLabelMapper->GetLabelTextProperty()->SetColor(1,1,1);
    this->AreaLabelMapper->GetLabelTextProperty()->SetJustificationToCentered();
    this->AreaLabelMapper->GetLabelTextProperty()->SetVerticalJustificationToCentered();
    this->AreaLabelMapper->GetLabelTextProperty()->SetFontSize(12);
    this->AreaLabelMapper->GetLabelTextProperty()->SetItalic(0);
    this->AreaLabelMapper->GetLabelTextProperty()->SetLineOffset(0);
    vtkDynamic2DLabelMapper* dynamic = vtkDynamic2DLabelMapper::SafeDownCast(mapper);
    if (dynamic)
      {
      dynamic->SetPriorityArrayName("GraphVertexDegree");
      }
    if (tmpMapper)
      {
      tmpMapper->Delete();
      }
    }
  else
    {
    vtkErrorMacro("Area label mapper must not be null.");
    }
}

// ----------------------------------------------------------------------
void vtkTreeAreaView::SetUseRectangularCoordinates(bool rect)
{
  vtkInteractorStyleAreaSelectHover* style =
    vtkInteractorStyleAreaSelectHover::SafeDownCast(this->InteractorStyle);
  if (style)
    {
    style->SetUseRectangularCoordinates(rect);
    }
}

// ----------------------------------------------------------------------
bool vtkTreeAreaView::GetUseRectangularCoordinates()
{
  vtkInteractorStyleAreaSelectHover* style =
    vtkInteractorStyleAreaSelectHover::SafeDownCast(this->InteractorStyle);
  if (style)
    {
    return style->GetUseRectangularCoordinates();
    }
  return false;
}

