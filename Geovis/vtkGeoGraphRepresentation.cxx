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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkGeoGraphRepresentation.h"

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkExtractSelectedGraph.h"
#include "vtkGeoArcs.h"
#include "vtkGeoAssignCoordinates.h"
#include "vtkGeometryFilter.h"
#include "vtkGraphToPolyData.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLabeledDataMapper.h"
#include "vtkLookupTable.h"
#include "vtkMaskPoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSelectVisiblePoints.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkViewTheme.h"
#include "vtkXMLDataSetWriter.h"

vtkCxxRevisionMacro(vtkGeoGraphRepresentation, "1.3");
vtkStandardNewMacro(vtkGeoGraphRepresentation);
//----------------------------------------------------------------------------
vtkGeoGraphRepresentation::vtkGeoGraphRepresentation()
{
  this->AssignCoordinates         = vtkGeoAssignCoordinates::New();
  this->GraphToPolyData           = vtkGraphToPolyData::New();
  this->GeoArcs                   = vtkGeoArcs::New();
  this->EdgeMapper                = vtkPolyDataMapper::New();
  this->EdgeActor                 = vtkActor::New();
  this->VertexGlyph               = vtkVertexGlyphFilter::New();
  this->VertexMapper              = vtkPolyDataMapper::New();
  this->VertexActor               = vtkActor::New();
  this->OutlineMapper             = vtkPolyDataMapper::New();
  this->OutlineActor              = vtkActor::New();
  this->VertexLabelMapper               = vtkLabeledDataMapper::New();
  this->VertexLabelMaskPoints           = vtkMaskPoints::New();
  this->VertexLabelSelectVisiblePoints  = vtkSelectVisiblePoints::New();
  this->VertexLabelTransform            = vtkTransformPolyDataFilter::New();
  this->VertexLabelActor                = vtkActor2D::New();
  this->EdgeCellCenters               = vtkCellCenters::New();
  this->EdgeLabelMapper               = vtkLabeledDataMapper::New();
  this->EdgeLabelMaskPoints           = vtkMaskPoints::New();
  this->EdgeLabelSelectVisiblePoints  = vtkSelectVisiblePoints::New();
  this->EdgeLabelTransform            = vtkTransformPolyDataFilter::New();
  this->EdgeLabelActor                = vtkActor2D::New();
  this->ExtractSelection          = vtkExtractSelectedGraph::New();
  this->SelectionAssignCoords     = vtkGeoAssignCoordinates::New();
  this->SelectionToPolyData       = vtkGraphToPolyData::New();
  this->SelectionGeoArcs          = vtkGeoArcs::New();
  this->SelectionMapper           = vtkPolyDataMapper::New();
  this->SelectionActor            = vtkActor::New();
  this->SelectionVertexGlyph      = vtkVertexGlyphFilter::New();
  this->SelectionVertexMapper     = vtkPolyDataMapper::New();
  this->SelectionVertexActor      = vtkActor::New();

  this->VertexColorArrayNameInternal = 0;
  this->EdgeColorArrayNameInternal = 0;
  
  // Connect pipeline
  this->GraphToPolyData->SetInputConnection(
    this->AssignCoordinates->GetOutputPort());
  this->GeoArcs->SetInputConnection(this->GraphToPolyData->GetOutputPort());
  this->EdgeMapper->SetInputConnection(this->GeoArcs->GetOutputPort());
  this->EdgeActor->SetMapper(this->EdgeMapper);
  this->VertexGlyph->SetInputConnection(this->GraphToPolyData->GetOutputPort());
  this->VertexMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->VertexActor->SetMapper(this->VertexMapper);
  this->OutlineMapper->SetInputConnection(this->VertexGlyph->GetOutputPort());
  this->OutlineActor->SetMapper(this->OutlineMapper);

  this->VertexLabelMaskPoints->SetInputConnection(
    this->GraphToPolyData->GetOutputPort());
  this->VertexLabelMaskPoints->RandomModeOn();
  this->VertexLabelMaskPoints->SetMaximumNumberOfPoints(75);
  this->VertexLabelMaskPoints->SetOnRatio(1);
  this->VertexLabelTransform->SetInputConnection(
    this->VertexLabelMaskPoints->GetOutputPort());
  vtkTransform *xform = vtkTransform::New();
  this->VertexLabelTransform->SetTransform(xform);
  xform->Delete();
  this->VertexLabelSelectVisiblePoints->SetInputConnection(
    this->VertexLabelTransform->GetOutputPort());
  this->VertexLabelMapper->SetInputConnection(
    this->VertexLabelSelectVisiblePoints->GetOutputPort());
  this->VertexLabelActor->SetMapper(this->VertexLabelMapper);

  this->EdgeCellCenters->SetInputConnection(
    this->GeoArcs->GetOutputPort());
  this->EdgeLabelMaskPoints->SetInputConnection(
    this->EdgeCellCenters->GetOutputPort());
  this->EdgeLabelMaskPoints->RandomModeOn();
  this->EdgeLabelMaskPoints->SetMaximumNumberOfPoints(75);
  this->EdgeLabelMaskPoints->SetOnRatio(1);
  this->EdgeLabelTransform->SetInputConnection(
    this->EdgeLabelMaskPoints->GetOutputPort());
  xform = vtkTransform::New();
  this->EdgeLabelTransform->SetTransform(xform);
  xform->Delete();
  this->EdgeLabelSelectVisiblePoints->SetInputConnection(
    this->EdgeLabelTransform->GetOutputPort());
  this->EdgeLabelMapper->SetInputConnection(
    this->EdgeLabelSelectVisiblePoints->GetOutputPort());
  this->EdgeLabelActor->SetMapper(this->EdgeLabelMapper);

  this->ExtractSelection->SetInputConnection(1,
    this->SelectionLink->GetOutputPort());
  this->SelectionAssignCoords->SetInputConnection(
    this->ExtractSelection->GetOutputPort());
  this->SelectionToPolyData->SetInputConnection(
    this->SelectionAssignCoords->GetOutputPort());
  this->SelectionGeoArcs->SetInputConnection(
    this->SelectionToPolyData->GetOutputPort());
  this->SelectionMapper->SetInputConnection(
    this->SelectionGeoArcs->GetOutputPort());
  this->SelectionActor->SetMapper(this->SelectionMapper);
  this->SelectionVertexGlyph->SetInputConnection(
    this->SelectionToPolyData->GetOutputPort());
  this->SelectionVertexMapper->SetInputConnection(
    this->SelectionVertexGlyph->GetOutputPort());
  this->SelectionVertexActor->SetMapper(this->SelectionVertexMapper);
  
  // Set parameters
  this->AssignCoordinates->SetLatitudeArrayName("latitude");
  this->AssignCoordinates->SetLongitudeArrayName("longitude");
  this->VertexMapper->SetScalarModeToUsePointData();
  this->VertexActor->GetProperty()->SetPointSize(5);
  this->OutlineMapper->ScalarVisibilityOff();
  this->VertexLabelMapper->SetLabelModeToLabelFieldData();
  vtkTextProperty* vertText = this->VertexLabelMapper->GetLabelTextProperty();
  vertText->SetColor(1,1,1);
  vertText->SetJustificationToCentered();
  vertText->SetVerticalJustificationToCentered();
  vertText->SetFontSize(12);
  vertText->SetItalic(0);
  vertText->SetLineOffset(-10);
  this->VertexLabelActor->PickableOff();
  this->VertexLabelActor->VisibilityOff();
  this->EdgeLabelMapper->SetLabelModeToLabelFieldData();
  vtkTextProperty* edgeText = this->EdgeLabelMapper->GetLabelTextProperty();
  edgeText->SetColor(1,1,1);
  edgeText->SetJustificationToCentered();
  edgeText->SetVerticalJustificationToCentered();
  edgeText->SetFontSize(12);
  edgeText->SetItalic(0);
  edgeText->SetLineOffset(-10);
  this->EdgeLabelActor->PickableOff();
  this->EdgeLabelActor->VisibilityOff();
  this->SelectionAssignCoords->SetLatitudeArrayName("latitude");
  this->SelectionAssignCoords->SetLongitudeArrayName("longitude");
  this->SelectionMapper->ScalarVisibilityOff();
  this->SelectionActor->GetProperty()->SetColor(1, 0, 1);
  this->SelectionActor->GetProperty()->SetRepresentationToWireframe();
  this->SelectionActor->PickableOff();
  this->SelectionVertexMapper->ScalarVisibilityOff();
  this->SelectionVertexActor->GetProperty()->SetPointSize(5);
  this->SelectionVertexActor->GetProperty()->SetColor(1, 0, 1);
  this->SelectionVertexActor->PickableOff();
}

//----------------------------------------------------------------------------
vtkGeoGraphRepresentation::~vtkGeoGraphRepresentation()
{
  this->AssignCoordinates->Delete();
  this->GraphToPolyData->Delete();
  this->GeoArcs->Delete();
  this->EdgeMapper->Delete();
  this->EdgeActor->Delete();
  this->VertexGlyph->Delete();
  this->VertexMapper->Delete();
  this->VertexActor->Delete();
  this->OutlineMapper->Delete();
  this->OutlineActor->Delete();
  this->VertexLabelMaskPoints->Delete();
  this->VertexLabelSelectVisiblePoints->Delete();
  this->VertexLabelTransform->Delete();
  this->VertexLabelMapper->Delete();
  this->VertexLabelActor->Delete();
  this->EdgeLabelMaskPoints->Delete();
  this->EdgeLabelSelectVisiblePoints->Delete();
  this->EdgeLabelTransform->Delete();
  this->EdgeCellCenters->Delete();
  this->EdgeLabelMapper->Delete();
  this->EdgeLabelActor->Delete();
  this->ExtractSelection->Delete();
  this->SelectionAssignCoords->Delete();
  this->SelectionToPolyData->Delete();
  this->SelectionGeoArcs->Delete();
  this->SelectionMapper->Delete();
  this->SelectionActor->Delete();
  this->SelectionVertexGlyph->Delete();
  this->SelectionVertexMapper->Delete();
  this->SelectionVertexActor->Delete();

  this->SetVertexColorArrayNameInternal(0);
  this->SetEdgeColorArrayNameInternal(0);
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetInputConnection(vtkAlgorithmOutput* conn)
{
  this->Superclass::SetInputConnection(conn);
  this->AssignCoordinates->SetInputConnection(conn);
  this->AssignCoordinates->GetInputConnection(0, 0);
  this->ExtractSelection->SetInputConnection(0, conn);
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetSelectionLink(vtkSelectionLink* link)
{
  this->Superclass::SetSelectionLink(link);
  this->ExtractSelection->SetInputConnection(1, link->GetOutputPort());
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetVertexLabelArrayName(const char* name)
{
  this->VertexLabelMapper->SetFieldDataName(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetVertexLabelArrayName()
{
  return this->VertexLabelMapper->GetFieldDataName();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetVertexLabelVisibility(bool b)
{
  this->VertexLabelActor->SetVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkGeoGraphRepresentation::GetVertexLabelVisibility()
{
  return (this->VertexLabelActor->GetVisibility() == 1);
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetExplodeFactor(double factor)
{
  this->GeoArcs->SetExplodeFactor(factor);
  this->SelectionGeoArcs->SetExplodeFactor(factor);
}

//----------------------------------------------------------------------------
double vtkGeoGraphRepresentation::GetExplodeFactor()
{
  return this->GeoArcs->GetExplodeFactor();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetNumberOfSubdivisions(int num)
{
  this->GeoArcs->SetNumberOfSubdivisions(num);
  this->SelectionGeoArcs->SetNumberOfSubdivisions(num);
}

//----------------------------------------------------------------------------
int vtkGeoGraphRepresentation::GetNumberOfSubdivisions()
{
  return this->GeoArcs->GetNumberOfSubdivisions();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetLatitudeArrayName(const char* name)
{
  this->AssignCoordinates->SetLatitudeArrayName(name);
  this->SelectionAssignCoords->SetLatitudeArrayName(name);
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
  this->SelectionAssignCoords->SetLongitudeArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetLongitudeArrayName()
{
  return this->AssignCoordinates->GetLongitudeArrayName();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetVertexLabelFontSize(int size)
{
  this->VertexLabelMapper->GetLabelTextProperty()->SetFontSize(size);
}

//----------------------------------------------------------------------------
int vtkGeoGraphRepresentation::GetVertexLabelFontSize()
{
  return this->VertexLabelMapper->GetLabelTextProperty()->GetFontSize();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetColorVertices(bool b)
{
  this->VertexMapper->SetScalarVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkGeoGraphRepresentation::GetColorVertices()
{
  return this->VertexMapper->GetScalarVisibility() ? true : false;
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetVertexColorArrayName(const char* name)
{
  this->SetVertexColorArrayNameInternal(name);
  this->VertexMapper->SetScalarModeToUsePointFieldData();
  this->VertexMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetVertexColorArrayName()
{
  return this->GetVertexColorArrayNameInternal();
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
  this->EdgeMapper->SetScalarVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkGeoGraphRepresentation::GetColorEdges()
{
  return (this->EdgeMapper->GetScalarVisibility() == 1);
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::SetEdgeColorArrayName(const char* name)
{
  this->SetEdgeColorArrayNameInternal(name);
  this->EdgeMapper->SetScalarModeToUseCellFieldData();
  this->EdgeMapper->SelectColorArray(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoGraphRepresentation::GetEdgeColorArrayName()
{
  return this->GetEdgeColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::ApplyViewTheme(vtkViewTheme* theme)
{
  this->VertexActor->GetProperty()->SetColor(theme->GetPointColor());
  this->VertexActor->GetProperty()->SetOpacity(theme->GetPointOpacity());
  this->VertexActor->GetProperty()->SetPointSize(theme->GetPointSize());
  //this->OutlineActor->GetProperty()->SetColor(theme->GetOutlineColor());

  vtkSmartPointer<vtkLookupTable> vertLUT =
    vtkSmartPointer<vtkLookupTable>::New();
  vertLUT->SetHueRange(theme->GetPointHueRange());
  vertLUT->SetSaturationRange(theme->GetPointSaturationRange()); 
  vertLUT->SetValueRange(theme->GetPointValueRange());
  vertLUT->SetAlphaRange(theme->GetPointAlphaRange());
  vertLUT->Build();
  this->VertexMapper->SetLookupTable(vertLUT);

  this->EdgeActor->GetProperty()->SetColor(theme->GetCellColor());
  this->EdgeActor->GetProperty()->SetOpacity(theme->GetCellOpacity());
  this->EdgeActor->GetProperty()->SetLineWidth(theme->GetLineWidth());
  vtkSmartPointer<vtkLookupTable> edgeLUT =
    vtkSmartPointer<vtkLookupTable>::New();
  edgeLUT->SetHueRange(theme->GetCellHueRange());
  edgeLUT->SetSaturationRange(theme->GetCellSaturationRange());
  edgeLUT->SetValueRange(theme->GetCellValueRange());
  edgeLUT->SetAlphaRange(theme->GetCellAlphaRange());
  edgeLUT->Build();
  this->EdgeMapper->SetLookupTable(edgeLUT);

  this->SelectionActor->GetProperty()->SetColor(
    theme->GetSelectedCellColor());
  this->SelectionActor->GetProperty()->SetOpacity(
    theme->GetSelectedCellOpacity());
  this->SelectionActor->GetProperty()->SetLineWidth(
    theme->GetLineWidth()+1);

  this->SelectionVertexActor->GetProperty()->SetColor(
    theme->GetSelectedPointColor());
  this->SelectionVertexActor->GetProperty()->SetOpacity(
    theme->GetSelectedPointOpacity());
  this->SelectionVertexActor->GetProperty()->SetPointSize(
    theme->GetPointSize()+4);

  this->OutlineActor->GetProperty()->SetColor(
    theme->GetOutlineColor());
  this->OutlineActor->GetProperty()->SetOpacity(
    theme->GetPointOpacity());
  this->OutlineActor->GetProperty()->SetPointSize(
    theme->GetPointSize()+2);
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
  this->VertexLabelSelectVisiblePoints->SetRenderer(rv->GetRenderer());
  this->EdgeLabelSelectVisiblePoints->SetRenderer(rv->GetRenderer());
  rv->GetRenderer()->AddActor(this->SelectionActor);
  rv->GetRenderer()->AddActor(this->SelectionVertexActor);
  rv->GetRenderer()->AddActor(this->OutlineActor);
  rv->GetRenderer()->AddActor(this->EdgeActor);
  rv->GetRenderer()->AddActor(this->EdgeLabelActor);
  rv->GetRenderer()->AddActor(this->VertexActor);
  rv->GetRenderer()->AddActor(this->VertexLabelActor);

  // Register progress.
  view->RegisterProgress(GraphToPolyData);
  view->RegisterProgress(GeoArcs);
  view->RegisterProgress(EdgeMapper);
  view->RegisterProgress(VertexGlyph);
  view->RegisterProgress(VertexMapper);
  view->RegisterProgress(OutlineMapper);
  view->RegisterProgress(VertexLabelMapper);
  view->RegisterProgress(VertexLabelMaskPoints);
  view->RegisterProgress(VertexLabelSelectVisiblePoints);
  view->RegisterProgress(VertexLabelTransform);
  view->RegisterProgress(EdgeCellCenters);
  view->RegisterProgress(EdgeLabelMapper);
  view->RegisterProgress(EdgeLabelMaskPoints);
  view->RegisterProgress(EdgeLabelSelectVisiblePoints);
  view->RegisterProgress(EdgeLabelTransform);
  view->RegisterProgress(SelectionAssignCoords);
  view->RegisterProgress(SelectionToPolyData);
  view->RegisterProgress(SelectionGeoArcs);
  view->RegisterProgress(SelectionMapper);
  view->RegisterProgress(SelectionVertexGlyph);
  view->RegisterProgress(SelectionVertexMapper);
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
  rv->GetRenderer()->RemoveActor(this->EdgeActor);
  rv->GetRenderer()->RemoveActor(this->EdgeLabelActor);
  rv->GetRenderer()->RemoveActor(this->VertexActor);
  rv->GetRenderer()->RemoveActor(this->OutlineActor);
  rv->GetRenderer()->RemoveActor(this->VertexLabelActor);
  rv->GetRenderer()->RemoveActor(this->SelectionActor);
  rv->GetRenderer()->RemoveActor(this->SelectionVertexActor);

  // UnRegister Progress
  view->UnRegisterProgress(GraphToPolyData);
  view->UnRegisterProgress(GeoArcs);
  view->UnRegisterProgress(EdgeMapper);
  view->UnRegisterProgress(VertexGlyph);
  view->UnRegisterProgress(VertexMapper);
  view->UnRegisterProgress(OutlineMapper);
  view->UnRegisterProgress(VertexLabelMapper);
  view->UnRegisterProgress(VertexLabelMaskPoints);
  view->UnRegisterProgress(VertexLabelSelectVisiblePoints);
  view->UnRegisterProgress(VertexLabelTransform);
  view->UnRegisterProgress(EdgeCellCenters);
  view->UnRegisterProgress(EdgeLabelMapper);
  view->UnRegisterProgress(EdgeLabelMaskPoints);
  view->UnRegisterProgress(EdgeLabelSelectVisiblePoints);
  view->UnRegisterProgress(EdgeLabelTransform);
  view->UnRegisterProgress(SelectionAssignCoords);
  view->UnRegisterProgress(SelectionToPolyData);
  view->UnRegisterProgress(SelectionGeoArcs);
  view->UnRegisterProgress(SelectionMapper);
  view->UnRegisterProgress(SelectionVertexGlyph);
  view->UnRegisterProgress(SelectionVertexMapper);
  return true;
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::PrepareForRendering()
{
  this->GeoArcs->Update();
  double range[2];
  vtkPolyData* poly = this->GeoArcs->GetOutput();
  vtkDataArray* colorArr = 0;
  if (this->GetColorEdges())
    {
    colorArr = poly->GetCellData()->GetArray(
      this->EdgeColorArrayNameInternal);
    }
  if (colorArr)
    {
    colorArr->GetRange(range);
    this->EdgeMapper->SetScalarRange(range);
    }
  poly = this->GraphToPolyData->GetOutput();
  colorArr = 0;
  if (this->GetColorVertices())
    {
    colorArr = poly->GetPointData()->GetArray(
      this->VertexColorArrayNameInternal);
    }
  if (colorArr)
    {
    colorArr->GetRange(range);
    this->VertexMapper->SetScalarRange(range);
    }
}

//----------------------------------------------------------------------------
vtkSelection* vtkGeoGraphRepresentation::ConvertSelection(
  vtkView* view, 
  vtkSelection* selection)
{

  vtkSmartPointer<vtkSelection> propSelection =
    vtkSmartPointer<vtkSelection>::New();
  // Look for the selection for this prop.
  if (selection->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) ==
      vtkSelection::SELECTIONS)
    {
    // First, try to find a vertex selection.
    bool found = false;
    for (unsigned int i = 0; i < selection->GetNumberOfChildren(); i++)
      {
      vtkSelection* child = selection->GetChild(i);
      vtkProp* prop = vtkProp::SafeDownCast(child->GetProperties()->Get(vtkSelection::PROP()));
      if (prop == this->VertexActor)
        {
        propSelection->ShallowCopy(child);
        propSelection->SetFieldType(vtkSelection::VERTEX);
        found = true;
        }
      }
    // If no vertex selection, look for an edge selection.
    if (!found)
      {
      for (unsigned int i = 0; i < selection->GetNumberOfChildren(); i++)
        {
        vtkSelection* child = selection->GetChild(i);
        vtkProp* prop = vtkProp::SafeDownCast(child->GetProperties()->Get(vtkSelection::PROP()));
        if (prop == this->EdgeActor)
          {
          propSelection->ShallowCopy(child);
          propSelection->SetFieldType(vtkSelection::EDGE);
          found = true;
          }
        }
      }
    }
  else
    {
    propSelection->ShallowCopy(selection);
    }

  // Start with an empty selection
  vtkSelection* converted = vtkSelection::New();
  converted->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), view->GetSelectionType());
  converted->GetProperties()->Set(
    vtkSelection::FIELD_TYPE(), vtkSelection::CELL);
  vtkSmartPointer<vtkIdTypeArray> empty =
    vtkSmartPointer<vtkIdTypeArray>::New();
  converted->SetSelectionList(empty);
  // Convert to the correct type of selection
  if (this->InputConnection)
    {
    vtkAlgorithm* producer = this->InputConnection->GetProducer();
    producer->Update();
    vtkDataObject* obj = producer->GetOutputDataObject(
      this->InputConnection->GetIndex());
    if (obj)
      {
      vtkSelection* index = vtkConvertSelection::ToSelectionType(
        propSelection, obj, view->GetSelectionType(),
        view->GetSelectionArrayNames());
      converted->ShallowCopy(index);
      index->Delete();
      }
    }

  return converted;
}

//----------------------------------------------------------------------------
void vtkGeoGraphRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "AssignCoordinates:" << endl;
  this->AssignCoordinates->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GraphToPolyData:" << endl;
  this->GraphToPolyData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GeoArcs:" << endl;
  this->GeoArcs->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeMapper:" << endl;
  this->EdgeMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexGlyph:" << endl;
  this->VertexGlyph->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexMapper:" << endl;
  this->VertexMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "OutlineMapper:" << endl;
  this->VertexLabelMaskPoints->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexLabelSelectVisiblePoints:" << endl;
  this->VertexLabelSelectVisiblePoints->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexLabelMapper:" << endl;
  this->VertexLabelMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "ExtractSelection:" << endl;
  this->ExtractSelection->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionAssignCoords:" << endl;
  this->SelectionAssignCoords->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionToPolyData:" << endl;
  this->SelectionToPolyData->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionGeoArcs:" << endl;
  this->SelectionGeoArcs->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionMapper:" << endl;
  this->SelectionMapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionVertexGlyph:" << endl;
  this->VertexGlyph->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionVertexMapper:" << endl;
  this->VertexMapper->PrintSelf(os, indent.GetNextIndent());
  if (this->GetInputConnection())
    {
    os << indent << "EdgeActor:" << endl;
    this->EdgeActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "EdgeLabelActor:" << endl;
    this->EdgeLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "VertexActor:" << endl;
    this->VertexActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "VertexLabelActor:" << endl;
    this->VertexLabelActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "SelectionActor:" << endl;
    this->SelectionActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "SelectionVertexActor:" << endl;
    this->SelectionVertexActor->PrintSelf(os, indent.GetNextIndent());
    this->OutlineMapper->PrintSelf(os, indent.GetNextIndent());
    os << indent << "OutlineActor:" << endl;
    this->OutlineActor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "VertexLabelMaskPoints:" << endl;
    }
}
