/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSurfaceRepresentation.cxx

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

#include "vtkSurfaceRepresentation.h"

#include "vtkActor.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkConvertSelection.h"
#include "vtkDataObject.h"
#include "vtkExtractSelection.h"
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

vtkCxxRevisionMacro(vtkSurfaceRepresentation, "1.5");
vtkStandardNewMacro(vtkSurfaceRepresentation);
//----------------------------------------------------------------------------
vtkSurfaceRepresentation::vtkSurfaceRepresentation()
{
  this->GeometryFilter          = vtkGeometryFilter::New();
  this->Mapper                  = vtkPolyDataMapper::New();
  this->Actor                   = vtkActor::New();
  this->ExtractSelection        = vtkExtractSelection::New();
  this->SelectionGeometryFilter = vtkGeometryFilter::New();
  this->SelectionMapper         = vtkPolyDataMapper::New();
  this->SelectionActor          = vtkActor::New();
  
  // Connect pipeline
  this->Mapper->SetInputConnection(this->GeometryFilter->GetOutputPort());
  this->Actor->SetMapper(this->Mapper);
  this->ExtractSelection->SetInputConnection(1, this->SelectionLink->GetOutputPort());
  this->SelectionGeometryFilter->SetInputConnection(this->ExtractSelection->GetOutputPort());
  this->SelectionMapper->SetInputConnection(this->SelectionGeometryFilter->GetOutputPort());
  this->SelectionActor->SetMapper(this->SelectionMapper);
  
  // Set parameters
  this->SelectionMapper->ScalarVisibilityOff();
  this->SelectionActor->GetProperty()->SetColor(1, 0, 1);
  this->SelectionActor->GetProperty()->SetRepresentationToWireframe();
  this->SelectionActor->PickableOff();
}

//----------------------------------------------------------------------------
vtkSurfaceRepresentation::~vtkSurfaceRepresentation()
{
  this->GeometryFilter->Delete();
  this->Mapper->Delete();
  this->Actor->Delete();
  this->ExtractSelection->Delete();
  this->SelectionGeometryFilter->Delete();
  this->SelectionMapper->Delete();
  this->SelectionActor->Delete();
}

//----------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetInputConnection(vtkAlgorithmOutput* conn)
{
  this->Superclass::SetInputConnection(conn);
  this->GeometryFilter->SetInputConnection(conn);
  this->ExtractSelection->SetInputConnection(conn);
}

//----------------------------------------------------------------------------
void vtkSurfaceRepresentation::SetSelectionLink(vtkSelectionLink* link)
{
  this->Superclass::SetSelectionLink(link);
  this->ExtractSelection->SetInputConnection(1, link->GetOutputPort());
}

//----------------------------------------------------------------------------
bool vtkSurfaceRepresentation::AddToView(vtkView* view)
{
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (!rv)
    {
    vtkErrorMacro("Can only add to a subclass of vtkRenderView.");
    return false;
    }
  rv->GetRenderer()->AddActor(this->Actor);
  rv->GetRenderer()->AddActor(this->SelectionActor);
  return true;
}

//----------------------------------------------------------------------------
bool vtkSurfaceRepresentation::RemoveFromView(vtkView* view)
{
  vtkRenderView* rv = vtkRenderView::SafeDownCast(view);
  if (!rv)
    {
    return false;
    }
  rv->GetRenderer()->RemoveActor(this->Actor);
  rv->GetRenderer()->RemoveActor(this->SelectionActor);
  return true;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSurfaceRepresentation::ConvertSelection(
  vtkView* view, 
  vtkSelection* selection)
{
  vtkSmartPointer<vtkSelection> propSelection =
    vtkSmartPointer<vtkSelection>::New();

  // Extract the selection for the right prop
  if (selection->GetContentType() == vtkSelection::SELECTIONS)
    {
    for (unsigned int i = 0; i < selection->GetNumberOfChildren(); i++)
      {
      vtkSelection* child = selection->GetChild(i);
      vtkProp* prop = vtkProp::SafeDownCast(
        child->GetProperties()->Get(vtkSelection::PROP()));
      if (prop == this->Actor)
        {
        propSelection->ShallowCopy(child);
        }
      }
    }
  else
    {
    propSelection->ShallowCopy(selection);
    }

  // Start with an empty selection
  vtkSelection* converted = vtkSelection::New();
  converted->SetContentType(view->GetSelectionType());
  converted->SetFieldType(vtkSelection::CELL);
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
void vtkSurfaceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "GeometryFilter:" << endl;
  this->GeometryFilter->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Mapper:" << endl;
  this->Mapper->PrintSelf(os, indent.GetNextIndent());
  os << indent << "SelectionGeometryFilter:" << endl;
  this->SelectionGeometryFilter->PrintSelf(os, indent.GetNextIndent());
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
