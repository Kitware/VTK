/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoGraphRepresentation.cxx

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

#include "vtkGeoGraphRepresentation.h"

#include "vtkAbstractTransform.h"
#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkArcParallelEdgeStrategy.h"
#include "vtkCamera.h"
#include "vtkCellCenters.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkEdgeCenters.h"
#include "vtkEdgeLayout.h"
#include "vtkEdgeListIterator.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkExtractSelection.h"
#include "vtkGeoAssignCoordinates.h"
#include "vtkGeoEdgeStrategy.h"
#include "vtkGeoView.h"
#include "vtkGraphMapper.h"
#include "vtkGraphToPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLabeledDataMapper.h"
#include "vtkLabelPlacer.h"
#include "vtkLabelSizeCalculator.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSetToLabelHierarchy.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkViewTheme.h"
#include "vtkXMLDataSetWriter.h"

vtkCxxRevisionMacro(vtkGeoGraphRepresentation, "1.22");
vtkStandardNewMacro(vtkGeoGraphRepresentation);
//----------------------------------------------------------------------------
vtkGeoGraphRepresentation::vtkGeoGraphRepresentation()
{
  this->AssignCoordinates         = vtkSmartPointer<vtkGeoAssignCoordinates>::New();
  this->EdgeLayout                = vtkSmartPointer<vtkEdgeLayout>::New();
  this->GraphMapper               = vtkSmartPointer<vtkGraphMapper>::New();
  this->GraphActor                = vtkSmartPointer<vtkActor>::New();
  this->GraphToPolyData           = vtkSmartPointer<vtkGraphToPolyData>::New();
  this->LabelSize                 = vtkSmartPointer<vtkLabelSizeCalculator>::New();
  this->LabelHierarchy            = vtkSmartPointer<vtkPointSetToLabelHierarchy>::New();
  this->LabelPlacer               = vtkSmartPointer<vtkLabelPlacer>::New();
  this->LabelMapper               = vtkSmartPointer<vtkLabeledDataMapper>::New();
  this->LabelActor                = vtkSmartPointer<vtkActor2D>::New();
  this->EdgeCenters                   = vtkSmartPointer<vtkEdgeCenters>::New();
  this->EdgeLabelMapper               = vtkSmartPointer<vtkLabeledDataMapper>::New();
  this->EdgeLabelTransform            = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
  this->EdgeLabelActor                = vtkSmartPointer<vtkActor2D>::New();
  this->ExtractSelection          = vtkSmartPointer<vtkExtractSelectedGraph>::New();
  this->SelectionMapper           = vtkSmartPointer<vtkGraphMapper>::New();
  this->SelectionActor            = vtkSmartPointer<vtkActor>::New();

  this->LabelArrayName = NULL;

  // Connect pipeline
  this->EdgeLayout->SetInputConnection(this->AssignCoordinates->GetOutputPort());
  this->GraphMapper->SetInputConnection(this->EdgeLayout->GetOutputPort());
  this->GraphActor->SetMapper(this->GraphMapper);
  this->ExtractSelection->SetInputConnection(0, this->EdgeLayout->GetOutputPort());
  this->SelectionMapper->SetInputConnection(this->ExtractSelection->GetOutputPort());
  this->SelectionActor->SetMapper(this->SelectionMapper);

  this->LabelSize->SetInputConnection(this->AssignCoordinates->GetOutputPort());
  this->LabelHierarchy->SetInputConnection(this->LabelSize->GetOutputPort());
  this->LabelPlacer->SetInputConnection(this->LabelHierarchy->GetOutputPort());
  this->LabelMapper->SetInputConnection(this->LabelPlacer->GetOutputPort());
  this->LabelActor->SetMapper(this->LabelMapper);

  this->EdgeCenters->SetInputConnection(this->EdgeLayout->GetOutputPort());
  this->EdgeLabelTransform->SetInputConnection(this->EdgeCenters->GetOutputPort());
  this->EdgeLabelMapper->SetInputConnection(this->EdgeLabelTransform->GetOutputPort());
  this->EdgeLabelActor->SetMapper(this->EdgeLabelMapper);
  
  // Set parameters
  vtkSmartPointer<vtkTextProperty> tp = vtkSmartPointer<vtkTextProperty>::New();
  tp->SetColor(1,1,1);
  tp->SetJustificationToCentered();
  tp->SetVerticalJustificationToCentered();
  tp->SetFontSize(12);
  tp->SetItalic(0);
  tp->SetBold(1);
  tp->SetShadow(1);
  tp->SetLineOffset(-10);
  this->LabelHierarchy->SetMaximumDepth(3);
  this->LabelHierarchy->
    SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, "LabelSize");
  this->SetVertexLabelArrayName("Label");
  // Turn off labels on the other side of the world
  this->LabelPlacer->PositionsAsNormalsOn();
  this->LabelMapper->SetFieldDataName("LabelText");
  this->LabelMapper->SetLabelModeToLabelFieldData();
  this->LabelMapper->SetLabelTextProperty(tp);
  this->LabelActor->PickableOff();
  this->LabelActor->VisibilityOff();
  this->SetEdgeLayoutStrategyToGeo();
  this->AssignCoordinates->SetLatitudeArrayName("latitude");
  this->AssignCoordinates->SetLongitudeArrayName("longitude");
  vtkSmartPointer<vtkTransform> edgeTrans = vtkSmartPointer<vtkTransform>::New();
  this->EdgeLabelTransform->SetTransform(edgeTrans);
  this->EdgeLabelMapper->SetLabelModeToLabelFieldData();
  vtkSmartPointer<vtkTextProperty> etp = vtkSmartPointer<vtkTextProperty>::New();
  etp->SetColor(0.8,0.5,1.0);
  etp->SetJustificationToCentered();
  etp->SetVerticalJustificationToCentered();
  etp->SetFontSize(10);
  etp->SetItalic(0);
  etp->SetBold(1);
  etp->SetShadow(1);
  etp->SetLineOffset(-10);
  this->EdgeLabelMapper->SetLabelTextProperty(etp);
  this->EdgeLabelActor->PickableOff();
  this->EdgeLabelActor->VisibilityOff();
  this->SelectionActor->GetProperty()->SetColor(1, 0, 1);
  this->SelectionActor->GetProperty()->SetRepresentationToWireframe();
  this->SelectionActor->PickableOff();

  // Variable to keep track of whether we are in a 3D geo view.
  this->In3DGeoView = false;
}

//----------------------------------------------------------------------------
vtkGeoGraphRepresentation::~vtkGeoGraphRepresentation()
{
  if(this->LabelArrayName != NULL)
    {
    delete [] this->LabelArrayName;
    }
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetupInputConnections()
{
  this->AssignCoordinates->SetInput(this->GetInput());
  this->ExtractSelection->SetInputConnection(1,
    this->GetSelectionConnection());
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetVertexLabelArrayName(const char* name)
{
  // Currently use the same array for priorities and labels.
  this->SetLabelArrayName(name);
  if (name)
    {
    this->LabelSize->SetInputArrayToProcess(0, 0, 0,
      vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
    this->LabelHierarchy->SetInputArrayToProcess(0, 0, 0,
      vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
    this->LabelHierarchy->SetInputArrayToProcess(2, 0, 0,
      vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
    }
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetVertexLabelArrayName()
{
  return this->GetLabelArrayName();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetVertexLabelVisibility(bool b)
{
  this->LabelActor->SetVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkGeoGraphRepresentation::GetVertexLabelVisibility()
{
  return (this->LabelActor->GetVisibility() == 1);
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetExplodeFactor(double factor)
{
  vtkGeoEdgeStrategy* geo = vtkGeoEdgeStrategy::SafeDownCast(
    this->GetEdgeLayoutStrategy());
  if (geo)
    {
    geo->SetExplodeFactor(factor);
    }
}

//----------------------------------------------------------------------------
double vtkGeoGraphRepresentation::GetExplodeFactor()
{
  vtkGeoEdgeStrategy* geo = vtkGeoEdgeStrategy::SafeDownCast(
    this->GetEdgeLayoutStrategy());
  if (geo)
    {
    return geo->GetExplodeFactor();
    }
  return 0.0;
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetNumberOfSubdivisions(int num)
{
  vtkGeoEdgeStrategy* geo = vtkGeoEdgeStrategy::SafeDownCast(
    this->GetEdgeLayoutStrategy());
  if (geo)
    {
    geo->SetNumberOfSubdivisions(num);
    }
}

//----------------------------------------------------------------------------
int vtkGeoGraphRepresentation::GetNumberOfSubdivisions()
{
  vtkGeoEdgeStrategy* geo = vtkGeoEdgeStrategy::SafeDownCast(
    this->GetEdgeLayoutStrategy());
  if (geo)
    {
    return geo->GetNumberOfSubdivisions();
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetLatitudeArrayName(const char* name)
{
  this->AssignCoordinates->SetLatitudeArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetLatitudeArrayName()
{
  return this->AssignCoordinates->GetLatitudeArrayName();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetLongitudeArrayName(const char* name)
{
  this->AssignCoordinates->SetLongitudeArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetLongitudeArrayName()
{
  return this->AssignCoordinates->GetLongitudeArrayName();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetVertexLabelFontSize(int size)
{
  this->LabelMapper->GetLabelTextProperty()->SetFontSize(size);
  this->LabelMapper->Modified();
}

//----------------------------------------------------------------------------
int vtkGeoGraphRepresentation::GetVertexLabelFontSize()
{
  return this->LabelMapper->GetLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetColorVertices(bool b)
{
  this->GraphMapper->SetColorVertices(b);
}

//----------------------------------------------------------------------------
bool vtkGeoGraphRepresentation::GetColorVertices()
{
  return this->GraphMapper->GetColorVertices();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetVertexColorArrayName(const char* name)
{
  this->GraphMapper->SetVertexColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetVertexColorArrayName()
{
  return this->GraphMapper->GetVertexColorArrayName();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetEdgeLabelVisibility(bool b)
{
  this->EdgeLabelActor->SetVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkGeoGraphRepresentation::GetEdgeLabelVisibility()
{
  return (this->EdgeLabelActor->GetVisibility() == 1);
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetEdgeLabelArrayName(const char* name)
{
  this->EdgeLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetEdgeLabelArrayName()
{
  return this->EdgeLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetEdgeLabelFontSize(int size)
{
  this->EdgeLabelMapper->GetLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int vtkGeoGraphRepresentation::GetEdgeLabelFontSize()
{
  return this->EdgeLabelMapper->GetLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetColorEdges(bool b)
{
  this->GraphMapper->SetColorEdges(b);
}

//----------------------------------------------------------------------------
bool vtkGeoGraphRepresentation::GetColorEdges()
{
  return this->GraphMapper->GetColorEdges();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetEdgeColorArrayName(const char* name)
{
  this->GraphMapper->SetEdgeColorArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetEdgeColorArrayName()
{
  return this->GraphMapper->GetEdgeColorArrayName();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetEdgeLayoutStrategy(vtkEdgeLayoutStrategy* strategy)
{
  this->EdgeLayout->SetLayoutStrategy(strategy);
}

//----------------------------------------------------------------------------
vtkEdgeLayoutStrategy* vtkGeoGraphRepresentation::GetEdgeLayoutStrategy()
{
  return this->EdgeLayout->GetLayoutStrategy();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetEdgeLayoutStrategyToGeo()
{
  if (!vtkGeoEdgeStrategy::SafeDownCast(this->GetEdgeLayoutStrategy()))
    {
    vtkSmartPointer<vtkGeoEdgeStrategy> s =
      vtkSmartPointer<vtkGeoEdgeStrategy>::New();
    this->SetEdgeLayoutStrategy(s);
    }
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetEdgeLayoutStrategyToArcParallel()
{
  if (!vtkArcParallelEdgeStrategy::SafeDownCast(this->GetEdgeLayoutStrategy()))
    {
    vtkSmartPointer<vtkArcParallelEdgeStrategy> s =
      vtkSmartPointer<vtkArcParallelEdgeStrategy>::New();
    this->SetEdgeLayoutStrategy(s);
    }
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::ApplyViewTheme(vtkViewTheme* theme)
{
  this->GraphMapper->ApplyViewTheme(theme);

  vtkSmartPointer<vtkViewTheme> selectTheme =
    vtkSmartPointer<vtkViewTheme>::New();
  selectTheme->SetPointColor(theme->GetSelectedPointColor());
  selectTheme->SetCellColor(theme->GetSelectedPointColor());
  selectTheme->SetOutlineColor(theme->GetSelectedPointColor());
  this->SelectionMapper->ApplyViewTheme(selectTheme);
  this->SelectionMapper->SetVertexPointSize(theme->GetPointSize()+4);
  this->SelectionMapper->SetEdgeLineWidth(theme->GetLineWidth()+3);
}

//----------------------------------------------------------------------------
bool vtkGeoGraphRepresentation::AddToView(vtkView* view)
{
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (!rv)
    {
    vtkErrorMacro("Can only add to a subclass of vtkRenderView.");
    return false;
    }
  if (vtkGeoView::SafeDownCast(view))
    {
    this->In3DGeoView = true;
    }
  this->LabelPlacer->SetRenderer(rv->GetRenderer());
  rv->GetRenderer()->AddActor(this->SelectionActor);
  rv->GetRenderer()->AddActor(this->GraphActor);
  rv->GetRenderer()->AddActor(this->EdgeLabelActor);
  rv->GetRenderer()->AddActor(this->LabelActor);

  // Register progress.
  view->RegisterProgress(AssignCoordinates);
  view->RegisterProgress(LabelSize);
  view->RegisterProgress(LabelHierarchy);
  view->RegisterProgress(LabelPlacer);
  view->RegisterProgress(LabelMapper);
  view->RegisterProgress(EdgeLayout);
  view->RegisterProgress(GraphMapper);
  view->RegisterProgress(GraphToPolyData);
  view->RegisterProgress(EdgeCenters);
  view->RegisterProgress(EdgeLabelMapper);
  view->RegisterProgress(EdgeLabelTransform);
  view->RegisterProgress(SelectionMapper);
  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoGraphRepresentation::RemoveFromView(vtkView* view)
{
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (!rv)
    {
    return false;
    }
  rv->GetRenderer()->RemoveActor(this->SelectionActor);
  rv->GetRenderer()->RemoveActor(this->GraphActor);
  rv->GetRenderer()->RemoveActor(this->EdgeLabelActor);
  rv->GetRenderer()->RemoveActor(this->LabelActor);

  // UnRegister Progress
  view->UnRegisterProgress(AssignCoordinates);
  view->UnRegisterProgress(LabelSize);
  view->UnRegisterProgress(LabelHierarchy);
  view->UnRegisterProgress(LabelPlacer);
  view->UnRegisterProgress(LabelMapper);
  view->UnRegisterProgress(EdgeLayout);
  view->UnRegisterProgress(GraphMapper);
  view->UnRegisterProgress(GraphToPolyData);
  view->UnRegisterProgress(EdgeCenters);
  view->UnRegisterProgress(EdgeLabelMapper);
  view->UnRegisterProgress(EdgeLabelTransform);
  view->UnRegisterProgress(SelectionMapper);
  return true;
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::PrepareForRendering()
{
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetTransform(vtkAbstractTransform* trans)
{
  this->AssignCoordinates->SetTransform(trans);
}

//----------------------------------------------------------------------------
vtkAbstractTransform* vtkGeoGraphRepresentation::GetTransform()
{
  return this->AssignCoordinates->GetTransform();
}

//----------------------------------------------------------------------------
vtkSelection* vtkGeoGraphRepresentation::ConvertSelection(
  vtkView* view, 
  vtkSelection* selection)
{
  // Convert from a frustum selection to a vertex index selection
  vtkSmartPointer<vtkSelection> pointSel = vtkSmartPointer<vtkSelection>::New();
  pointSel->ShallowCopy(selection);
  pointSel->GetNode(0)->SetFieldType(vtkSelectionNode::POINT);
  vtkSmartPointer<vtkGraphToPolyData> poly = vtkSmartPointer<vtkGraphToPolyData>::New();
  poly->SetInputConnection(this->AssignCoordinates->GetOutputPort());
  vtkSmartPointer<vtkExtractSelection> extract = vtkSmartPointer<vtkExtractSelection>::New();
  extract->SetInputConnection(0, poly->GetOutputPort());
  extract->SetInput(1, pointSel);
  extract->Update();
  vtkDataSet* extractedData = vtkDataSet::SafeDownCast(extract->GetOutput());
  vtkAbstractArray* extractPedIds = extractedData->GetPointData()->GetPedigreeIds();

  vtkSmartPointer<vtkIdTypeArray> facingIds = vtkSmartPointer<vtkIdTypeArray>::New();
  vtkSmartPointer<vtkIdTypeArray> edgeIds = vtkSmartPointer<vtkIdTypeArray>::New();
  if (extractPedIds)
    {
    // Extract the vertices on the correct side of the globe
    vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
    vtkCamera* cam = rv->GetRenderer()->GetActiveCamera();
    double pos[3];
    double pt[3];
    cam->GetPosition(pos);
    vtkGraph* graph = vtkGraph::SafeDownCast(this->AssignCoordinates->GetOutput());
    vtkAbstractArray* graphPedIds = graph->GetVertexData()->GetPedigreeIds();
    vtkIdType numTuples = extractPedIds->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numTuples; ++i)
      {
      vtkVariant v = extractPedIds->GetVariantValue(i);
      vtkIdType vertex = graphPedIds->LookupValue(v);
      if (vertex < 0)
        {
        continue;
        }
      graph->GetPoint(vertex, pt);
      if (!this->In3DGeoView || vtkMath::Dot(pos, pt) > 0.0)
        {
        facingIds->InsertNextValue(graphPedIds->LookupValue(v));
        }
      }

    // Extract edges connecting selected vertices
    vtkSmartPointer<vtkEdgeListIterator> it = vtkSmartPointer<vtkEdgeListIterator>::New();
    graph->GetEdges(it);
    while (it->HasNext())
      {
      vtkEdgeType e = it->Next();
      if (facingIds->LookupValue(e.Source) >= 0 && facingIds->LookupValue(e.Target) >= 0)
        {
        edgeIds->InsertNextValue(e.Id);
        }
      }
    }
  vtkSmartPointer<vtkSelectionNode> vertSel = vtkSmartPointer<vtkSelectionNode>::New();
  vertSel->SetSelectionList(facingIds);
  vertSel->SetContentType(vtkSelectionNode::INDICES);
  vertSel->SetFieldType(vtkSelectionNode::VERTEX);

  vtkSmartPointer<vtkSelectionNode> edgeSel = vtkSmartPointer<vtkSelectionNode>::New();
  edgeSel->SetSelectionList(edgeIds);
  edgeSel->SetContentType(vtkSelectionNode::INDICES);
  edgeSel->SetFieldType(vtkSelectionNode::EDGE);

  vtkSmartPointer<vtkSelection> parentSel = vtkSmartPointer<vtkSelection>::New();
  parentSel->AddNode(vertSel);
  parentSel->AddNode(edgeSel);

  // Convert to the selection type needed for this view
  vtkSmartPointer<vtkSelection> conv;
  conv.TakeReference(vtkConvertSelection::ToSelectionType(
    parentSel, this->AssignCoordinates->GetOutput(), view->GetSelectionType(),
    view->GetSelectionArrayNames()));

  vtkSelection* converted = vtkSelection::New();
  converted->ShallowCopy(conv);
  return converted;
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AssignCoordinates:" << endl;
  this->AssignCoordinates->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeLayout:" << endl;
  this->EdgeLayout->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphMapper:" << endl;
  this->GraphMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphToPolyData:" << endl;
  this->GraphToPolyData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ExtractSelection:" << endl;
  this->ExtractSelection->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionMapper:" << endl;
  this->SelectionMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LabelHierarchy:" << endl;
  this->LabelHierarchy->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LabelPlacer:" << endl;
  this->LabelPlacer->PrintSelf(os, indent.GetNextIndent());
  os << indent << "LabelMapper:" << endl;
  this->LabelMapper->PrintSelf(os, indent.GetNextIndent());
  if (this->GetInputConnection())
    {
    os << indent << "GraphActor:" << endl;
    this->GraphActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "LabelActor:" << endl;
    this->LabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeLabelActor:" << endl;
    this->EdgeLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "SelectionActor:" << endl;
    this->SelectionActor->PrintSelf(os, indent.GetNextIndent());
    }
}
