/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoLineRepresentation.cxx

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

#include "vtkGeoLineRepresentation.h"

#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkExtractSelection.h"
#include "vtkGeoAdaptiveArcs.h"
#include "vtkGeoAssignCoordinates.h"
#include "vtkGeoMath.h"
#include "vtkGeoSampleArcs.h"
#include "vtkGeometryFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderView.h"
#include "vtkSelection.h"
#include "vtkSelectionLink.h"
#include "vtkSmartPointer.h"
#include "vtkVertexGlyphFilter.h"
#include "vtkXMLDataSetWriter.h"

vtkCxxRevisionMacro(vtkGeoLineRepresentation, "1.4");
vtkStandardNewMacro(vtkGeoLineRepresentation);
//----------------------------------------------------------------------------
vtkGeoLineRepresentation::vtkGeoLineRepresentation()
{
  this->GeometryFilter          = vtkGeometryFilter::New();
  this->AssignCoordinates       = vtkGeoAssignCoordinates::New();
  this->GeoArcs                 = vtkGeoAdaptiveArcs::New();
  this->GeoSampleArcs           = vtkGeoSampleArcs::New();
  this->Mapper                  = vtkPolyDataMapper::New();
  this->Actor                   = vtkActor::New();
  this->VertexGlyphFilter       = vtkVertexGlyphFilter::New();
  this->VertexMapper            = vtkPolyDataMapper::New();
  this->VertexActor             = vtkActor::New();
  this->ExtractSelection        = vtkExtractSelection::New();
  this->SelectionGeometryFilter = vtkGeometryFilter::New();
  this->SelectionAssignCoords   = vtkGeoAssignCoordinates::New();
  this->SelectionGeoArcs        = vtkGeoAdaptiveArcs::New();
  this->SelectionGeoSampleArcs  = vtkGeoSampleArcs::New();
  this->SelectionMapper         = vtkPolyDataMapper::New();
  this->SelectionActor          = vtkActor::New();
  
  // Connect pipeline
  this->AssignCoordinates->SetInputConnection(this->GeometryFilter->GetOutputPort());
  //this->GeoArcs->SetInputConnection(this->AssignCoordinates->GetOutputPort());
  this->GeoSampleArcs->SetInputConnection(this->AssignCoordinates->GetOutputPort());
  //this->Mapper->SetInputConnection(this->GeoArcs->GetOutputPort());
  this->Mapper->SetInputConnection(this->GeoSampleArcs->GetOutputPort());
  this->Actor->SetMapper(this->Mapper);
  //this->VertexGlyphFilter->SetInputConnection(this->GeoArcs->GetOutputPort());
  this->VertexGlyphFilter->SetInputConnection(this->GeoSampleArcs->GetOutputPort());
  this->VertexMapper->SetInputConnection(this->VertexGlyphFilter->GetOutputPort());
  this->VertexActor->SetMapper(this->VertexMapper);
  this->ExtractSelection->SetInputConnection(1, this->GetSelectionConnection());
  this->SelectionGeometryFilter->SetInputConnection(this->ExtractSelection->GetOutputPort());
  this->SelectionAssignCoords->SetInputConnection(this->SelectionGeometryFilter->GetOutputPort());
  //this->SelectionGeoArcs->SetInputConnection(this->SelectionAssignCoords->GetOutputPort());
  this->SelectionGeoSampleArcs->SetInputConnection(this->SelectionAssignCoords->GetOutputPort());
  //this->SelectionMapper->SetInputConnection(this->SelectionGeoArcs->GetOutputPort());
  this->SelectionMapper->SetInputConnection(this->SelectionGeoSampleArcs->GetOutputPort());
  this->SelectionActor->SetMapper(this->SelectionMapper);
  
  // Set parameters
  this->AssignCoordinates->SetLatitudeArrayName("latitude");
  this->AssignCoordinates->SetLongitudeArrayName("longitude");
  //this->GeoArcs->SetGlobeRadius(vtkGeoMath::EarthRadiusMeters()*1.0001);
  this->GeoSampleArcs->SetGlobeRadius(vtkGeoMath::EarthRadiusMeters()*1.0001);
  this->Actor->GetProperty()->SetColor(0, 0, 0);
  // Make this type of representation non-selectable
  // because it is used for political boundaries.
  this->Actor->PickableOff();
  this->VertexActor->GetProperty()->SetPointSize(5);
  this->VertexActor->GetProperty()->SetColor(1, 0, 0);
  this->VertexActor->VisibilityOff();
  this->SelectionAssignCoords->SetLatitudeArrayName("latitude");
  this->SelectionAssignCoords->SetLongitudeArrayName("longitude");
  this->SelectionMapper->ScalarVisibilityOff();
  this->SelectionActor->GetProperty()->SetColor(1, 0, 1);
  this->SelectionActor->GetProperty()->SetRepresentationToWireframe();
  this->SelectionActor->PickableOff();

  // This normally represents static lines like political boundaries,
  // so turn off selectability by default.
  this->SelectableOff();
}

//----------------------------------------------------------------------------
vtkGeoLineRepresentation::~vtkGeoLineRepresentation()
{
  this->GeometryFilter->Delete();
  this->AssignCoordinates->Delete();
  this->GeoArcs->Delete();
  this->GeoSampleArcs->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->VertexGlyphFilter->Delete();
  this->VertexMapper->Delete();
  this->VertexActor->Delete();
  this->ExtractSelection->Delete();
  this->SelectionGeometryFilter->Delete();
  this->SelectionAssignCoords->Delete();
  this->SelectionGeoArcs->Delete();
  this->SelectionGeoSampleArcs->Delete();
  this->SelectionMapper->Delete();
  this->SelectionActor->Delete();
}

//----------------------------------------------------------------------------
void vtkGeoLineRepresentation::SetInputConnection(vtkAlgorithmOutput* conn)
{
  this->Superclass::SetInputConnection(conn);
  this->GeometryFilter->SetInputConnection(conn);
  this->ExtractSelection->SetInputConnection(conn);
}

//----------------------------------------------------------------------------
void vtkGeoLineRepresentation::SetLatitudeArrayName(const char* name)
{
  this->AssignCoordinates->SetLatitudeArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoLineRepresentation::GetLatitudeArrayName()
{
  return this->AssignCoordinates->GetLatitudeArrayName();
}

//----------------------------------------------------------------------------
void vtkGeoLineRepresentation::SetLongitudeArrayName(const char* name)
{
  this->AssignCoordinates->SetLongitudeArrayName(name);
}

//----------------------------------------------------------------------------
const char* vtkGeoLineRepresentation::GetLongitudeArrayName()
{
  return this->AssignCoordinates->GetLongitudeArrayName();
}

//----------------------------------------------------------------------------
void vtkGeoLineRepresentation::SetPointVisibility(bool b)
{
  this->VertexActor->SetVisibility(b);
}

//----------------------------------------------------------------------------
bool vtkGeoLineRepresentation::GetPointVisibility()
{
  return (this->VertexActor->GetVisibility() != 0);
}

//----------------------------------------------------------------------------
void vtkGeoLineRepresentation::SetCoordinatesInArrays(bool b)
{
  this->AssignCoordinates->SetCoordinatesInArrays(b);
  this->SelectionAssignCoords->SetCoordinatesInArrays(b);
}

//----------------------------------------------------------------------------
bool vtkGeoLineRepresentation::GetCoordinatesInArrays()
{
  return this->AssignCoordinates->GetCoordinatesInArrays();
}

//----------------------------------------------------------------------------
bool vtkGeoLineRepresentation::AddToView(vtkView* view)
{
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (!rv)
    {
    vtkErrorMacro("Can only add to a subclass of vtkRenderView.");
    return false;
    }
  rv->GetRenderer()->AddActor(this->Actor);
  rv->GetRenderer()->AddActor(this->VertexActor);
  rv->GetRenderer()->AddActor(this->SelectionActor);
  this->GeoArcs->SetRenderer(rv->GetRenderer());
  this->SelectionGeoArcs->SetRenderer(rv->GetRenderer());

  view->RegisterProgress(this->GeometryFilter);
  view->RegisterProgress(this->AssignCoordinates);
  view->RegisterProgress(this->GeoArcs);
  view->RegisterProgress(this->GeoSampleArcs);
  view->RegisterProgress(this->Mapper);
  view->RegisterProgress(this->VertexGlyphFilter);
  view->RegisterProgress(this->VertexMapper);
  view->RegisterProgress(this->ExtractSelection);
  view->RegisterProgress(this->SelectionGeometryFilter);
  view->RegisterProgress(this->SelectionAssignCoords);
  view->RegisterProgress(this->SelectionGeoArcs);
  view->RegisterProgress(this->SelectionGeoSampleArcs);
  view->RegisterProgress(this->SelectionMapper);

  return true;
}

//----------------------------------------------------------------------------
bool vtkGeoLineRepresentation::RemoveFromView(vtkView* view)
{
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (!rv)
    {
    return false;
    }
  rv->GetRenderer()->RemoveActor(this->Actor);
  rv->GetRenderer()->RemoveActor(this->VertexActor);
  rv->GetRenderer()->RemoveActor(this->SelectionActor);

  view->UnRegisterProgress(this->GeometryFilter);
  view->UnRegisterProgress(this->AssignCoordinates);
  view->UnRegisterProgress(this->GeoArcs);
  view->UnRegisterProgress(this->GeoSampleArcs);
  view->UnRegisterProgress(this->Mapper);
  view->UnRegisterProgress(this->VertexGlyphFilter);
  view->UnRegisterProgress(this->VertexMapper);
  view->UnRegisterProgress(this->ExtractSelection);
  view->UnRegisterProgress(this->SelectionGeometryFilter);
  view->UnRegisterProgress(this->SelectionAssignCoords);
  view->UnRegisterProgress(this->SelectionGeoArcs);
  view->UnRegisterProgress(this->SelectionGeoSampleArcs);
  view->UnRegisterProgress(this->SelectionMapper);
  return true;
}

//----------------------------------------------------------------------------
void vtkGeoLineRepresentation::PrepareForRendering()
{
  //this->GeoArcs->Update();
  this->GeoSampleArcs->Update();
  //vtkPolyData* poly = this->GeoArcs->GetOutput();
  vtkPolyData* poly = this->GeoSampleArcs->GetOutput();
  vtkDataArray* scalars = poly->GetCellData()->GetScalars();
  if (scalars)
    {
    double range[2];
    scalars->GetRange(range);
    this->Mapper->SetScalarRange(range);
    }
  //vtkXMLDataSetWriter* writer = vtkXMLDataSetWriter::New();
  //writer->SetInputConnection(this->ExtractSelection->GetOutputPort());
  //writer->SetFileName("extract.xml");
  //writer->Write();
}

//----------------------------------------------------------------------------
vtkSelection* vtkGeoLineRepresentation::ConvertSelection(
  vtkView* vtkNotUsed(view), 
  vtkSelection* selection)
{
  // Make an empty selection
  vtkSelection* converted = vtkSelection::New();
  converted->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), vtkSelection::INDICES);
  converted->GetProperties()->Set(vtkSelection::FIELD_TYPE(), vtkSelection::CELL);
  vtkIdTypeArray* empty = vtkIdTypeArray::New();
  converted->SetSelectionList(empty);
  empty->Delete();

  if (selection->GetProperties()->Get(vtkSelection::CONTENT_TYPE()) == vtkSelection::SELECTIONS)
    {
    for (unsigned int i = 0; i < selection->GetNumberOfChildren(); i++)
      {
      vtkSelection* child = selection->GetChild(i);
      vtkProp* prop = vtkProp::SafeDownCast(child->GetProperties()->Get(vtkSelection::PROP()));
      if (prop == this->Actor)
        {
        // TODO: Should convert this to a pedigree id selection.
        converted->ShallowCopy(child);
        }
      }
    }
  
  return converted;
}

//----------------------------------------------------------------------------
void vtkGeoLineRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GeometryFilter:" << endl;
  this->GeometryFilter->PrintSelf(os, indent.GetNextIndent());
  os << indent << "AssignCoordinates:" << endl;
  this->AssignCoordinates->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GeoArcs:" << endl;
  this->GeoArcs->PrintSelf(os, indent.GetNextIndent());
  os << indent << "GeoSampleArcs:" << endl;
  this->GeoSampleArcs->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Mapper:" << endl;
  this->Mapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionGeometryFilter:" << endl;
  this->SelectionGeometryFilter->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionAssignCoords:" << endl;
  this->SelectionAssignCoords->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionGeoArcs:" << endl;
  this->SelectionGeoArcs->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionGeoSampleArcs:" << endl;
  this->SelectionGeoSampleArcs->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionMapper:" << endl;
  this->SelectionMapper->PrintSelf(os, indent.GetNextIndent());
  if (this->GetInputConnection())
    {
    os << indent << "Actor:" << endl;
    this->Actor->PrintSelf(os, indent.GetNextIndent());
    os << indent << "SelectionActor:" << endl;
    this->SelectionActor->PrintSelf(os, indent.GetNextIndent());
    }
}
